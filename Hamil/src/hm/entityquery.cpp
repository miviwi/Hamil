#include <hm/entityquery.h>
#include <hm/queryparams.h>

#include <cassert>
#include <cstdio>

namespace hm {

EntityQuery EntityQuery::empty_query(EntityManagerKey)
{
  return EntityQuery();
}

EntityQuery& EntityQuery::withNoAccess(
    ComponentTypeList all, ComponentTypeList any, ComponentTypeList none,
    EntityManagerKey)
{
  return withConditions<AccessNone>(
      { all.begin(), all.size() }, { any.begin(), any.size() }, { none.begin(), none.size() }
 );
}

EntityQuery& EntityQuery::withReadOnly(
    ComponentTypeList all, ComponentTypeList any, ComponentTypeList none,
    EntityManagerKey)
{
  return withConditions<AccessReadOnly>(
      { all.begin(), all.size() }, { any.begin(), any.size() }, { none.begin(), none.size() }
  );
}

EntityQuery& EntityQuery::withWriteOnly(
    ComponentTypeList all, ComponentTypeList any, ComponentTypeList none,
    EntityManagerKey)
{
  return withConditions<AccessWriteOnly>(
      { all.begin(), all.size() }, { any.begin(), any.size() }, { none.begin(), none.size() }
  );
}

EntityQuery& EntityQuery::withReadWrite(
    ComponentTypeList all, ComponentTypeList any, ComponentTypeList none,
    EntityManagerKey)
{
  return withConditions<AccessReadWrite>(
      { all.begin(), all.size() }, { any.begin(), any.size() }, { none.begin(), none.size() }
  );
}

EntityQuery& EntityQuery::withAccessByPtr(
    ComponentAccess access,
    ComponentTypeListPtr all, ComponentTypeListPtr any, ComponentTypeListPtr none, EntityManagerKey)
{
  switch(access) {
  case ComponentAccess::None:      return withConditions<AccessNone>(all, any, none);
  case ComponentAccess::ReadOnly:  return withConditions<AccessReadOnly>(all, any, none);
  case ComponentAccess::WriteOnly: return withConditions<AccessWriteOnly>(all, any, none);
  case ComponentAccess::ReadWrite: return withConditions<AccessReadWrite>(all, any, none);

  default: assert(0 && "unreachable");
  }
}

EntityQuery& EntityQuery::withConditionsByAccess(
    const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none,
    ConditionAccessMode access)
{

  return *this;
}

}
