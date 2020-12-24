#include <hm/entityman.h>
#include <hm/componentman.h>
#include <hm/prototype.h>
#include <hm/componentref.h>
#include <hm/components/gameobject.h>

#include <util/lfsr.h>
#include <util/hashindex.h>

#include <utility>
#include <functional>
#include <vector>

namespace hm {

struct EntityMeta {
  u32 proto_cache_id;    // Used to retrieve the CachedPrototype containing
                         //   the PrototypeChunk which holds the Component data
                         //   of the Entity

  u32 entity_id;    // The id assigned to this entity by allocEntity()/reassigned
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

class EntityManager : public IEntityManager {
public:
  enum {
    InitialEntities = 1024,
  };

  EntityManager(IComponentManager::Ptr component_man);

  virtual Entity createEntity();
  virtual Entity findEntity(const std::string& name);
  virtual void destroyEntity(EntityId id);

  virtual Entity createGameObject(const std::string& name, Entity parent);

  virtual bool alive(EntityId id);

private:
  EntityId newId();

  bool compareEntity(u32 key, u32 index);
  u32 findEntity(EntityId id);

  IComponentManager::Ptr m_component_man;

  util::MaxLength32BitLFSR m_next_id;

  util::HashIndex m_entities_hash;
  std::vector<EntityId> m_entities;

  std::function<bool(u32, u32)> m_cmp_entity_fn;
};

EntityManager::EntityManager(IComponentManager::Ptr component_man) :
  m_component_man(component_man),
  m_entities_hash(InitialEntities, InitialEntities)
{
  m_entities.reserve(InitialEntities);

  using std::placeholders::_1;
  using std::placeholders::_2;

  m_cmp_entity_fn = std::bind(&EntityManager::compareEntity, this, _1, _2);
}

Entity EntityManager::createEntity()
{
  auto id  = newId();
  auto idx = (util::HashIndex::Index)m_entities.size();

  m_entities.push_back(id);
  m_entities_hash.add(id, idx);

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

  Entity e = m_entities[idx];
  e.removeComponent<GameObject>();

  m_entities[idx] = Entity::Invalid;
}

Entity EntityManager::createGameObject(const std::string& name, Entity parent)
{
  auto e = createEntity();
  e.addComponent<GameObject>(name, parent.id());

  return e;
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
  return m_entities[index] == key;
}

u32 EntityManager::findEntity(EntityId id)
{
  return m_entities_hash.find(id, m_cmp_entity_fn);
}

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man)
{
  IEntityManager::Ptr ptr;
  ptr.reset(new EntityManager(component_man));

  return ptr;
}

}
