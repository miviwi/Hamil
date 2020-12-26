#include <hm/entityman.h>
#include <hm/componentman.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/cachedprototype.h>
#include <hm/prototypechunk.h>
#include <hm/componentref.h>
#include <hm/components/gameobject.h>

#include <util/lfsr.h>
#include <util/hashindex.h>

#include <limits>
#include <utility>
#include <functional>
#include <vector>

#include <cassert>

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

class EntityManager final : public IEntityManager {
public:
  enum {
    InitialEntities = 1024,
  };

  EntityManager(IComponentManager::Ptr component_man);

  virtual CachedPrototype prototype(const EntityPrototype& proto) final;

  virtual Entity createEntity(CachedPrototype proto) final;
  virtual Entity findEntity(const std::string& name) final;
  virtual void destroyEntity(EntityId id) final;

  virtual bool alive(EntityId id) final;

private:
  EntityId newId();

  bool compareEntity(u32 key, u32 index);
  u32 findEntity(EntityId id);

  util::MaxLength32BitLFSR m_next_id;

  EntityPrototypeCache m_proto_cache;

  util::HashIndex m_entities_hash;
  std::vector<EntityMeta> m_entities_meta;


  // ------- Old cruft :) ---------
  //  TODO: cleanup!

  //IComponentManager::Ptr m_component_man;



  std::function<bool(u32, u32)> m_cmp_entity_fn;
};

EntityManager::EntityManager(IComponentManager::Ptr component_man) :
  m_entities_hash(InitialEntities, InitialEntities)
{
  m_entities_meta.reserve(InitialEntities);

  /*
  using std::placeholders::_1;
  using std::placeholders::_2;

  m_cmp_entity_fn = std::bind(&EntityManager::compareEntity, this, _1, _2);
  */
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
  EntityMeta meta;

  auto id = newId();

  // Crude way to check if 'proto' is owned by this EntityManager
  //   - In REALITY only checks if the cache has been primed
  //     with an equivalent prototype, but this should be good
  //     enough to catch 90% of programmer mistakes
  assert(m_proto_cache.probe(proto.prototype()).has_value());

  auto alloc_id = proto.allocEntity();
  if(alloc_id == CachedPrototype::AllocEntityInvalidId) {
    // No more space in the current chunk - alloc a fresh one and retry
    proto.allocChunk();

    alloc_id = proto.allocEntity();   // Retry with fresh chunk...
  }

  assert(alloc_id != CachedPrototype::AllocEntityInvalidId);

  meta.id = id;
  meta.proto_cache_id = proto.cacheId();
  meta.alloc_id = alloc_id;

  assert(m_entities_meta.size() < std::numeric_limits<util::HashIndex::Index>::max());

  // assert() above ensures truncation won't occur
  auto idx = (util::HashIndex::Index)m_entities_meta.size();

  m_entities_meta.push_back(meta);
  m_entities_hash.add(id, idx);

  printf(
      "EntityManager::createEntity() =>\n"
      "    id=0x%.8x proto_cache_id=0x%.4x alloc_id=0x%.8x\n"
      "    index<EntityMeta>=%u\n\n",
      id, proto.cacheId(), alloc_id,
      idx
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
  auto idx = findEntity(id);
  if(idx == util::HashIndex::Invalid) return;

  STUB();

  //Entity e = m_entities[idx];
  //e.removeComponent<GameObject>();

  auto& meta = m_entities_meta.at(idx);

  meta.id = Entity::Invalid;
  meta.alloc_id = CachedPrototype::AllocEntityInvalidId;
  meta.proto_cache_id = EntityPrototypeCache::ProtoCacheIdInvalid;
}

bool EntityManager::alive(EntityId id)
{
  return findEntity(id) != util::HashIndex::Invalid;
}

EntityId EntityManager::newId()
{
  return m_next_id.next();
}

bool EntityManager::compareEntity(u32 key, u32 index)
{
  //return m_entities[index] == key;
  return false;
}

u32 EntityManager::findEntity(EntityId id)
{
  // Equivalent to util::HashIndex::find() call of the form
  //    => return m_entities_hash.find(id, [](auto key, auto index) -> bool { ... });
#if 1
  auto index = m_entities_hash.first(id);
  while(index != m_entities_hash.Invalid) {
    if(compareEntity(id, index)) {
      return index;     // A match!
    }

    index = m_entities_hash.next(index);    //  ...iterate through the 'chain'
  }

  return index;
#else
  return m_entities_hash.find(id, [this](EntityId key, u32 index) -> bool {
      return compareEntity(key, index);
  });
#endif
}

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man)
{
  IEntityManager::Ptr ptr;
  ptr.reset(new EntityManager(component_man));

  return ptr;
}

}
