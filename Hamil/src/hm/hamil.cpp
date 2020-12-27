#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>

#include <hm/components/all.h>

#include <util/format.h>

namespace hm {

IEntityManager::Ptr p_entity_man;

void init()
{
  p_entity_man = create_entity_manager();
}

void finalize()
{
  p_entity_man.reset();
}

IComponentManager& components()
{
  STUB();

  return *(IComponentManager *)nullptr;
}

IEntityManager& entities()
{
  return *p_entity_man;
}

void frame()
{
}

}
