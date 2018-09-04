#include <game/game.h>
#include <game/entity.h>
#include <game/entityman.h>
#include <game/component.h>
#include <game/componentman.h>

#include <game/components/all.h>

#include <util/format.h>

namespace game {

IComponentManager::Ptr p_component_man;
IEntityManager::Ptr p_entity_man;

void init()
{
  p_component_man = create_component_manager();
  p_entity_man = create_entity_manager(p_component_man);

  for(int i = 0; i < 5; i++) {
    auto name = util::fmt("test%d", i);

    entities().createEntity().addComponent<GameObject>(name);
  }

  Entity(0xA000'0D0F).removeComponent<GameObject>();

  entities().destroyEntity(1);

  components().foreach<GameObject>([](ComponentRef<GameObject> obj) {
    printf("0x%.8x: %s\n", obj().entity().id(), obj().name().data());
  });
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
