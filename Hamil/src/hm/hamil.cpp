#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>
#include <hm/componentman.h>

#include <hm/components/all.h>

#include <util/format.h>

namespace hm {

IComponentManager::Ptr p_component_man;
IEntityManager::Ptr p_entity_man;

void init()
{
  p_component_man = create_component_manager();
  p_entity_man = create_entity_manager(p_component_man);
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
