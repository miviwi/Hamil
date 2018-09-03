#include <game/entityman.h>
#include <game/componentman.h>

#include <game/components/testcomponent.h>

#include <util/lfsr.h>

#include <utility>

namespace game {

class EntityManager : public IEntityManager {
public:
  EntityManager(IComponentManager::Ptr component_man);

  virtual Entity createEntity();

private:
  EntityId newId();

  IComponentManager::Ptr m_component_man;

  util::MaxLength32BitLFSR m_next_id;
};

EntityManager::EntityManager(IComponentManager::Ptr component_man) :
  m_component_man(component_man)
{
}

Entity EntityManager::createEntity()
{
  Entity e(newId());

  auto c = m_component_man->createComponent<TestComponent>(e.id(), "hello :)");

  return e;
}

EntityId EntityManager::newId()
{
  return m_next_id.next();
}

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man)
{
  IEntityManager::Ptr ptr;
  ptr.reset(new EntityManager(component_man));

  return ptr;
}

}