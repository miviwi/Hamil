#include <game/game.h>
#include <game/entity.h>
#include <game/entityman.h>
#include <game/component.h>
#include <game/componentman.h>

#include <game/components/testcomponent.h>

namespace game {

IComponentManager::Ptr p_component_man;
IEntityManager::Ptr p_entity_man;

void init()
{
  p_component_man = create_component_manager();
  p_entity_man = create_entity_manager(p_component_man);

  auto e = entities().createEntity();

  printf("TestComponent(0x%.8x): %s\n", e.id(), e.component<TestComponent>()->name);
}

void finalize()
{
  p_entity_man.reset();
  p_component_man.reset();
}

IComponentManager& components()
{
  return *p_component_man;
}

IEntityManager& entities()
{
  return *p_entity_man;
}

void frame()
{
}

}
