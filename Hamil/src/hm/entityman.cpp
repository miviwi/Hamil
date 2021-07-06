#include <hm/entityman.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/cachedprototype.h>
#include <hm/prototypechunk.h>
#include <hm/chunkman.h>
#include <hm/componentref.h>
#include <hm/entityquery.h>
#include <hm/queryparams.h>

#include <util/lfsr.h>
#include <util/hashindex.h>

#include <limits>
#include <utility>
#include <functional>
#include <optional>
#include <vector>

#include <cassert>
#include <components.h>

namespace hm {

struct EntityMeta {
  u32 id;

  u32 proto_cache_id;    // Used to retrieve the CachedPrototype containing
                         //   the PrototypeChunk which holds the Component data
                         //   of the Entity

  u32 alloc_id;     // The id assigned to this entity by allocEntity()/reassigned
                    //   during chunk compaction, prototype promotion/demotion etc.
                    //   used to index into it's CachedPrototype's chunk array
                    //   and into the data slots of the chunk
                    //  - Can be used to retrieve the enclosing chunk via
                    //        CachedPrototype::chunkForEntityId(),
                    //    or a pointer to a particular Component's data via
                    //        CachedPrototype::componentDataForEntityId(),
                    //    if QnD access (ex. in non-performance sensitive code
                    //    paths) is good enough
};

struct PrototypeGroupChunk {
  u64 group_id;    // Calculated like so:
                   //    interleave_dwords(
                   //      proto.numChunks() - 1,  /* value at time of allocEntity() */
                   //      proto.cacheId()
                   //    )
                   //  where:
                   //    - 'proto' is the CachedPrototype for this group,
                   //    - interleave_dwords(x, y) [x,y are uint32_t's] yields:
                   //          y31 | ... | y3 | x3 | y2 | x2 | y1 | x1 | y0 | y0
                   //       where A[i] is the i'th bit of 'A' (ex. x2 == 3rd bit of x)

  u32 group_entities_offset;    // Offset into 'm_entities_meta' where the group's
                                //   Entities are stored; ordered as they are in
                                //   the PrototypeChunk

  u32 chunk_base_index;   // Stores a copy of PrototypeChunkHeader.base_offset
                          //   taken from the group's corresponding chunk
};

static inline u64 interleave_dword_with_0(u32 dword)
{
  u64 qword = dword;

  qword = (qword ^ (qword << 16ull)) & 0x0000'FFFF'0000'FFFFull;
  qword = (qword ^ (qword <<  8ull)) & 0x00FF'00FF'00FF'00FFull;
  qword = (qword ^ (qword <<  4ull)) & 0x0F0F'0F0F'0F0F'0F0Full;
  qword = (qword ^ (qword <<  2ull)) & 0x3333'3333'3333'3333ull;
  qword = (qword ^ (qword <<  1ull)) & 0x5555'5555'5555'5555ull;

  return qword;
}

static inline u64 interleave_dwords(u32 even, u32 odd)
{
  return interleave_dword_with_0(even) | (interleave_dword_with_0(odd) << 1ull);
}

class EntityManager final : public IEntityManager {
public:
  enum {
    InitialEntities = 1024,
    InitialEntityGroups = 64,
  };

  EntityManager();

  virtual CachedPrototype prototype(const EntityPrototype& proto) final;

  virtual Entity createEntity(CachedPrototype proto) final;
  virtual Entity findEntity(const std::string& name) final;
  virtual void destroyEntity(EntityId id) final;

  virtual bool alive(EntityId id) final;

  virtual IEntityManager& injectChunkManager(ChunkManager *chunk_man) final;

  virtual EntityQuery createEntityQuery(const EntityQueryParams& params) final;

private:
  EntityId newId();

  u32 entityMetaIdxById(EntityId id);

  // Calculates 'group_id' for the specified CachedPrototype's tail chunk
  //   - proto.numChunks() MUST be > 0 (i.e. at least one chunk
  //     was allocated) before calling this method!
  [[nodiscard]] u64 groupIdForTailChunkOf(const CachedPrototype& proto) const;

  // Calculates 'group_id' of Entity with specified EntityPrototype,
  //   stored at allocation-index 'alloc_id' of the prototype
  //  - Does no validation of 'alloc_id' - careful!
  u64 groupIdForProtoAndAllocId(const CachedPrototype& proto, u32 alloc_id);

  // Returns a pointer to the descriptor of the CURRENT 'tail' chunk
  //   in the specified prototype's group
  //  - Can return 'nullptr' if the descriptor's index can't be found
  //    in 'm_entity_groups_hash'
  PrototypeGroupChunk *tailChunkProtoGroupOf(const CachedPrototype& proto);

  PrototypeGroupChunk *protoGroupChunkById(u64 group_id);

  // Given a CachedPrototype handle and an 'alloc_id' of an Entity,
  //   retrieve it's EntityId (EntityMeta.id) by querying for the
  //   owning PrototypeGroupChunk's descriptor and using it's
  //   'group_entities_offset' field to index into -> 'm_entities_meta'
  EntityId entityIdByProtoAndAllocId(const CachedPrototype& proto, u32 alloc_id);
  EntityId entityIdByProtoAndChunkRelOffset(const CachedPrototype& proto, u32 off);

  util::MaxLength32BitLFSR m_next_id;

  ChunkManager *m_chunk_man = nullptr;

  EntityPrototypeCache m_proto_cache;

  util::HashIndex m_entities_hash;
  std::vector<EntityMeta> m_entities_meta;

  // EntityId -> (proto_cache_id, alloc_id) lookups are facilitated by
  //   maintaining a util::HashIndex where Entity ids are used as keys
  //   and are mapped into EntityMeta[] indices
  // However, it turns out that also supporting fast REVERSE lookups (so
  //   from (prototype_cache_id, alloc_id) -> EntityId) is crucial for
  //   operations such as destroying an Entity, promotion/demotion
  //   (adding and removing Components - respectively), and any others
  //   which shuffle data inside a chunk/between chunks; as all these
  //   will happen continually at non-trivial rates during simulation
  //   having a sub-optimal reverse EntityId lookup (such as a linear
  //   search) could cripple the whole system's performance.
  // The obvious/naive way of keeping a 'mirror' [index]->[EntityId]
  //   util::HashIndex for this purpose turns out impossible, unfortunately,
  //   as this would require always storing EntityMeta data for entities
  //   grouped by prototype (need to map 'alloc_id' -> EntityMeta[index],
  //   which cannot be done using a formula without such an invariant),
  //   to maintain this either indirection (= cache thrashing/misses!)
  //   or severe memory waste would have to be introduced, both reducing
  //   performance to unacceptable levels
  // Thus a more complex scheme must be employed. First, we associate a
  //   so-called 'group_id' with every Entity. It's computed from the index
  //   of it's chunk and prototype's id (see PrototypeGroupChunk::group_id
  //   definition above for more details), which makes it equal for Entities
  //   sharing the same chunk - providing an implicit grouping (hence it's
  //   name - 'group_id'). Secondly, instead of acquiring EntityMeta structs
  //   one by one (1 createEntity() == 1 m_entities_meta.push_back()), an
  //   entire chunks-worth is acquired alongside the allocChunk() call and
  //   later assigned directly according to the chunk-relative entity offsets.
  //   Finally we maintain a new mapping:
  //            'group_id' -> EntityMeta[group_starting_offset]
  //   which when combined with storing the EntityMeta structs grouped is
  //   enough to establish the reverse EntityId mapping; all while having
  //   introduced minimal per-Entity overhead (computing 'group_id') when
  //   creating/destroying them etc. and per-chunk overhead to maintain
  //   the new 'group_id'->group_start_offset mapping.
  //   On the other hand - this mapping uses an order of magnitude less
  //   memory compared to the mapping of the 'naive' way as we store
  //   per-chunk granularity data vs. per-entity for 'naive' mapping.
  //   Significant because cache pressure/pollution is reduced.
  // XXX: This scheme increases code complexity significantly enough to warrant
  //   supplying this rationale

  util::HashIndex m_entity_groups_hash;
  std::vector<PrototypeGroupChunk> m_entity_groups;
};

EntityManager::EntityManager() :
  m_entities_hash(InitialEntities, InitialEntities),
  m_entity_groups_hash(InitialEntityGroups, InitialEntityGroups)
{
  m_entities_meta.reserve(InitialEntities);
  m_entity_groups.reserve(InitialEntityGroups);
}

CachedPrototype EntityManager::prototype(const EntityPrototype& proto)
{
  // Return an existing cache line if 'proto' is in the cache
  if(auto cached = m_proto_cache.probe(proto)) {
    return cached.value();
  }

  return m_proto_cache.fill(proto);
}

Entity EntityManager::createEntity(CachedPrototype proto)
{
  assert(m_chunk_man && "createEntity() called before a ChunkManager was injected!");

  auto id = newId();

  // Crude way to check if 'proto' is owned by this EntityManager
  //   - In REALITY only checks if the cache has been primed
  //     with an equivalent prototype, but this should be good
  //     enough to catch 90% of programmer mistakes
  assert(m_proto_cache.probe(proto.prototype()).has_value());

  std::optional<PrototypeChunkHandle> chunk = std::nullopt;
  auto alloc_id = proto.allocEntity();
  if(alloc_id == CachedPrototype::AllocEntityInvalidId) {
    // No more space in the current chunk - alloc a fresh one and retry
    chunk = proto.allocChunk(m_chunk_man);

    alloc_id = proto.allocEntity();   // Retry with fresh chunk...
  }

  assert(alloc_id != CachedPrototype::AllocEntityInvalidId);

  auto tail_chunk_idx = proto.numChunks() - 1;
  auto chunk_capacity = proto.prototype().chunkCapacity();

  // Entities offset relative to it's enclosing chunk
  auto entity_index_in_chunk = alloc_id - tail_chunk_idx*chunk_capacity;

  PrototypeGroupChunk *entity_group_chunk = nullptr;
  if(chunk.has_value()) {     // Check if a fresh chunk had to be allocated...
    auto entity_groups_meta_off = m_entities_meta.size();
    auto group_chunk_idx = m_entity_groups.size();

    // Append a new PrototypeGroupChunk descriptor for
    //   the freshly-alllocated PrototypeChunk...
    auto& group_chunk = m_entity_groups.emplace_back();

    assert(entity_groups_meta_off < std::numeric_limits<u32>::max());

    //  ...and populate it's fields
    group_chunk.group_id = groupIdForTailChunkOf(proto);

    group_chunk.group_entities_offset = (u32)entity_groups_meta_off;
    group_chunk.chunk_base_index = chunk->entityBaseIndex();

    // Finally - store it's index in group_id -> PrototypeGroupChunk util::HashIndex
    m_entity_groups_hash.add((u32)group_chunk.group_id, group_chunk_idx);

    // Expand the EntityMeta array to accomadate the new chunk's capacity
    m_entities_meta.resize(entity_groups_meta_off + chunk->capacity());

    entity_group_chunk = &group_chunk;
  } else {
    // The tail chunk still had some space left-over
    entity_group_chunk = tailChunkProtoGroupOf(proto);
  }

  // Sanity check
  assert(entity_group_chunk);

  auto meta_idx = entity_group_chunk->group_entities_offset + entity_index_in_chunk;
  auto& meta = m_entities_meta.at(meta_idx);

  meta.id = id;
  meta.proto_cache_id = proto.cacheId();
  meta.alloc_id = alloc_id;

  assert(m_entities_meta.size() < std::numeric_limits<util::HashIndex::Index>::max());

  // assert() above ensures truncation won't occur
  //auto idx = (util::HashIndex::Index)m_entities_meta.size();

  //m_entities_meta.push_back(meta);
  m_entities_hash.add(id, meta_idx);

  /*
  auto tail_group_idx_of_groups = m_entity_groups_hash.find(tail_group_id,
      [this](u32 key, u32 index) { return m_entity_groups.at(index).
      */

  printf(
      "EntityManager::createEntity() =>\n"
      "    id=0x%.8x proto_cache_id=0x%.4x alloc_id=0x%.8x\n"
      "    index<EntityMeta>=%lu\n"
      "    entity_group_chunk[%p]:\n"
      "        .group_id=0x%.16lx\n"
      "        .group_entities_offset=0x%.8x\n"
      "\n",
      id, proto.cacheId(), alloc_id,
      meta_idx,
      entity_group_chunk,
      entity_group_chunk->group_id, entity_group_chunk->group_entities_offset
  );

  printf(
      "groupIdForProtoAndAllocId(proto, alloc_id=0x%.8x)=%.16lx\n"
      "entityIdByProtoAndAllocId(proto, alloc_id)=0x%.8x\n"
      "\n",
      meta.alloc_id,
      groupIdForProtoAndAllocId(proto, meta.alloc_id),
      entityIdByProtoAndAllocId(proto, meta.alloc_id)
  );

  return id;
}

Entity EntityManager::findEntity(const std::string& name)
{
  Entity e = Entity::Invalid;
  /*
  components().foreach([&](ComponentRef<GameObject> game_object) {
    if(game_object().name() == name) {
      e = game_object().entity();
    }
  });
  */

  return e;
}

void EntityManager::destroyEntity(EntityId id)
{
  STUB();

  //Entity e = m_entities[idx];
  //e.removeComponent<GameObject>();

  //auto& meta = m_entities_meta.at(idx);

  //meta.id = Entity::Invalid;
  //meta.alloc_id = CachedPrototype::AllocEntityInvalidId;
  //meta.proto_cache_id = EntityPrototypeCache::ProtoCacheIdInvalid;
}

bool EntityManager::alive(EntityId id)
{
  auto meta_idx = entityMetaIdxById(id);
  if(meta_idx == util::HashIndex::Invalid) return false;  // Can't be alive if it
                                                          //   never existed
  const auto& meta = m_entities_meta[meta_idx];

  return meta.alloc_id != CachedPrototype::AllocEntityInvalidId;
}

IEntityManager& EntityManager::injectChunkManager(ChunkManager *chunk_man)
{
  assert(!m_chunk_man &&
      "EntityManager::injectChunkManager() can be called only ONCE for a given manager instance");

  m_chunk_man = chunk_man;

  return *this;
}

EntityQuery EntityManager::createEntityQuery(const EntityQueryParams& params_)
{
  const auto& params = (const IEntityQueryParams&)params_;

  auto q = EntityQuery::empty_query();

  foreach_component_access([this,&params,&q](ComponentAccess access) {
    auto [ offset, length ] = params.componentsForAccess(access);
    if(!length) return;   // Skip empty groups

    using ComponentAccessGroup = util::SmallVector<ComponentProtoId>;
    std::array<ComponentAccessGroup, 3> access_groups = {
        ComponentAccessGroup(),   // QueryKind::AllOf
        ComponentAccessGroup(),   // QueryKind::AnyOf
        ComponentAccessGroup(),   // QueryKind::NoneOf
    };

    // For each component in group...
    for(unsigned i = 0; i < length; i++) {
      auto component_idx = offset+i;
      auto [ kind, component ] = params.componentByIndex(component_idx);

      switch(kind) {
      case IEntityQueryParams::AllOf:  access_groups[0].append(component); break;
      case IEntityQueryParams::AnyOf:  access_groups[1].append(component); break;
      case IEntityQueryParams::NoneOf: access_groups[2].append(component); break;
      }
    }

    printf("%u => {\n"
        "\tQueryKind::AllOf=(",
        (unsigned)access
    );
    for(auto c : access_groups[0]) printf("%s, ", protoid_to_str(c).data());
    printf(")\n");
    printf("\tQueryKind::AnyOf=(");
    for(auto c : access_groups[1]) printf("%s, ", protoid_to_str(c).data());
    printf(")\n");
    printf("\tQueryKind::NoneOf=(");
    for(auto c : access_groups[2]) printf("%s, ", protoid_to_str(c).data());
    printf(")\n"
           "}\n");

    q.withAccessByPtr(access,
        { access_groups[0].data(), access_groups[0].size() },
        { access_groups[1].data(), access_groups[1].size() },
        { access_groups[2].data(), access_groups[2].size() }
    );



  });

  //q.withReadOnly({ ComponentProto::GameObject, ComponentProto::Transform });

  return q;
}

EntityId EntityManager::newId()
{
  return m_next_id.next();
}

u32 EntityManager::entityMetaIdxById(EntityId id)
{
  // XXX: Confirm from disassembly that the loop from find() gets inlined
  //  along with the 'compare' callback invocations inside it,
  //  otherwise do it 'by hand'...
  return m_entities_hash.find(id,
      [this](EntityId id, u32 index) -> bool { return m_entities_meta[index].id == id; }
  );
}

u64 EntityManager::groupIdForTailChunkOf(const CachedPrototype& proto) const
{
  auto protocid = proto.cacheId();
  auto tail_chunk_idx = proto.numChunks() - 1;

  assert(tail_chunk_idx < std::numeric_limits<u32>::max());

  return interleave_dwords((u32)tail_chunk_idx, protocid);
}

PrototypeGroupChunk *EntityManager::tailChunkProtoGroupOf(const CachedPrototype& proto)
{
  auto group_id = groupIdForTailChunkOf(proto);

  return protoGroupChunkById(group_id);
}

u64 EntityManager::groupIdForProtoAndAllocId(const CachedPrototype& proto, u32 alloc_id)
{
  auto protocid = proto.cacheId();

  auto cached_proto = m_proto_cache.protoByCacheId(protocid);
  auto chunk = cached_proto.chunkForEntityAllocId(alloc_id);

  auto chunk_idx = alloc_id / (u32)chunk.capacity();

  return interleave_dwords(chunk_idx, protocid);
}

PrototypeGroupChunk *EntityManager::protoGroupChunkById(u64 group_id)
{
  // TODO: would something more clever than just chopping off topmost bits help?
  auto gid = (u32)group_id;

  auto group_idx = m_entity_groups_hash.find((u32)gid,
      [this,group_id](u32 key, u32 index) -> bool {
        return m_entity_groups[index].group_id == group_id;
      }
  );
  if(group_idx == util::HashIndex::Invalid) return nullptr;

  return &m_entity_groups[group_idx];
}

EntityId EntityManager::entityIdByProtoAndAllocId(
    const CachedPrototype& proto, u32 alloc_id
  )
{
  auto group_id = groupIdForProtoAndAllocId(proto, alloc_id);

  auto group_chunk = protoGroupChunkById(group_id);
  if(!group_chunk) return Entity::Invalid;

  auto group_meta_off = group_chunk->group_entities_offset;
  auto entity_off_in_chunk = alloc_id - group_chunk->chunk_base_index;

  const auto& meta = m_entities_meta.at(group_meta_off + entity_off_in_chunk);

  return meta.id;
}

EntityId EntityManager::entityIdByProtoAndChunkRelOffset(
    const CachedPrototype& proto, u32 chunk_rel_off
  )
{
  auto protocid = proto.cacheId();
  auto group_id = interleave_dwords(chunk_rel_off, protocid);

  auto group_chunk = protoGroupChunkById(group_id);
  if(!group_chunk) return Entity::Invalid;

  auto group_meta_off = group_chunk->group_entities_offset;

  const auto& meta = m_entities_meta.at(group_meta_off + chunk_rel_off);

  return meta.id;
}

IEntityManager::Ptr create_entity_manager()
{
  IEntityManager::Ptr ptr;
  ptr.reset(new EntityManager());

  return ptr;
}

}
