#include <game/game.h>
#include <game/entity.h>
#include <game/component.h>
#include <game/componentman.h>

#include <game/components/testcomponent.h>

namespace game {

std::unique_ptr<IComponentManager> p_component_man;

void init()
{
  p_component_man = create_component_manager();

  p_component_man->getComponentById<TestComponent>(1);
}

void finalize()
{
}

void frame()
{
}

}
