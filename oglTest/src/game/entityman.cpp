#include <game/entityman.h>
#include <game/componentman.h>

#include <util/lfsr.h>
#include <util/hashindex.h>

#include <utility>
#include <functional>
#include <vector>

namespace game {

class EntityManager : public IEntityManager {
public:
  enum {
    InitialEntities = 256,
  };

  EntityManager(IComponentManager::Ptr component_man);

  virtual Entity createEntity();
  virtual void destroyEntity(EntityId id);

  virtual bool alive(EntityId id);

private:
  EntityId newId();

  bool compareEntity(u32 key, u32 index);
  u32 findEntity(EntityId id);

  IComponentManager::Ptr m_component_man;

  util::MaxLength32BitLFSR m_next_id;

  util::HashIndex m_entities_hash;
  std::vector<EntityId> m_entities;
};

EntityManager::EntityManager(IComponentManager::Ptr component_man) :
  m_component_man(component_man),
  m_entities_hash(InitialEntities, InitialEntities)
{
  m_entities.reserve(InitialEntities);
}

Entity EntityManager::createEntity()
{
  auto id  = newId();
  auto idx = (util::HashIndex::Index)m_entities.size();

  m_entities.push_back(id);
  m_entities_hash.add(id, idx);

  return id;
}

void EntityManager::destroyEntity(EntityId id)
{
  auto idx = findEntity(id);
  if(idx == util::HashIndex::Invalid) return;

  m_entities[idx] = Entity::Invalid;
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
  using std::placeholders::_1;
  using std::placeholders::_2;

  return m_entities_hash.find(id, std::bind(&EntityManager::compareEntity, this, _1, _2));
}

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man)
{
  IEntityManager::Ptr ptr;
  ptr.reset(new EntityManager(component_man));

  return ptr;
}

}