#include <hm/entityquery.h>
#include <hm/queryparams.h>
#include <hm/entityman.h>

#include <components.h>

#include <config>

#include <cassert>
#include <cstdio>

namespace hm {

EntityQuery EntityQuery::empty_query(IEntityManager *entity_man, EntityManagerKey)
{
  return EntityQuery(entity_man);
}

EntityQuery::EntityQuery(IEntityManager *entity_man) :
  m_entity(entity_man),
  m_union_op({ ComponentTypeMap::zero(), ComponentTypeMap::zero(), ComponentTypeMap::zero() }),
  m_access_mask({ ComponentTypeMap::zero(), ComponentTypeMap::zero() })
{
}

const EntityQuery& EntityQuery::foreachComponent(ComponentIterFn&& fn) const
{
  const auto joint_mask = jointComponentTypeMask();

  auto has_component = [this,&fn](u64 bit) {
    auto component = (ComponentProtoId)bit;

    auto union_op = static_cast<UnionOp>(
          (m_union_op.all.test(bit) ? UnionAll : 0)
        + (m_union_op.any.test(bit) ? UnionAny : 0)
        + (m_union_op.none.test(bit) ? UnionNone : 0)
    );
    assert(union_op > UnionInvalid && union_op <= UnionNone);

    auto access = static_cast<ConditionAccessMode>(
        (m_access_mask.read.test(bit) ? AccessReadOnly : 0)
      | (m_access_mask.write.test(bit) ? AccessWriteOnly : 0));

    fn(component, union_op, from_access_mode(access));
  };

  foreach_bit_qword(joint_mask.bits.lo, 0, has_component);
  foreach_bit_qword(joint_mask.bits.hi, 64, has_component);

  return *this;
}

const EntityQuery& EntityQuery::foreachComponentGroupedByAccess(ComponentIterFn&& fn) const
{
  const auto joint_mask = jointComponentTypeMask();

  // ~(AccessRead & AccessWrite)   =>   1's everywhere !AccessRead && !AccessWrite
  const auto access_none_mask = m_access_mask.read.bitAnd(m_access_mask.write)
      .bitNot();

  // AccessRead & ~AccessWrite
  const auto access_ronly_mask = m_access_mask.read
      .bitAnd(m_access_mask.write.bitNot());
  // AccessWrite & ~AccessRead
  const auto access_wonly_mask = m_access_mask.write
      .bitAnd(m_access_mask.read.bitNot());

  // AccessRead & AccessWrite
  const auto access_rw_mask = m_access_mask.read.bitAnd(m_access_mask.write);

  auto do_access_group = [this](
      ComponentAccess access, ComponentTypeMap mask, const ComponentIterFn& fn)
  {
    assert((unsigned)access < (unsigned)ComponentAccess::NumAccessTypes);

    auto has_component = [this,&fn,access](u64 bit) {
      auto component = (ComponentProtoId)bit;

      auto union_op = static_cast<UnionOp>(
          (m_union_op.all.test(bit) ? UnionAll : 0)
              + (m_union_op.any.test(bit) ? UnionAny : 0)
              + (m_union_op.none.test(bit) ? UnionNone : 0)
      );
      assert(union_op > UnionInvalid && union_op <= UnionNone);

      fn(component, union_op, access);
    };

    foreach_bit_qword(mask.bits.lo, 0, has_component);
    foreach_bit_qword(mask.bits.hi, 64, has_component);
  };

  do_access_group(ComponentAccess::ReadOnly, joint_mask.bitAnd(access_ronly_mask), fn);
  do_access_group(ComponentAccess::WriteOnly, joint_mask.bitAnd(access_wonly_mask), fn);
  do_access_group(ComponentAccess::ReadWrite, joint_mask.bitAnd(access_rw_mask), fn);
  do_access_group(ComponentAccess::None, joint_mask.bitAnd(access_none_mask), fn);

  return *this;
}

void EntityQuery::dbg_PrintQueryComponents(bool group_by_access) const
{
  auto component_iter_fn = [](
      ComponentProtoId c, EntityQuery::UnionOp op, ComponentAccess access)
  {
    const char *op_str = "";
    switch(op) {
    case EntityQuery::UnionAll: op_str = "EntityQuery::UnionAll"; break;
    case EntityQuery::UnionAny: op_str = "EntityQuery::UnionAny"; break;
    case EntityQuery::UnionNone: op_str = "EntityQuery::UnionNone"; break;
    }

    const char *access_str = "";
    switch(access) {
    case ComponentAccess::None: access_str = "ComponentAccess::None"; break;
    case ComponentAccess::ReadOnly: access_str = "ComponentAccess::ReadOnly"; break;
    case ComponentAccess::WriteOnly: access_str = "ComponentAccess::WriteOnly"; break;
    case ComponentAccess::ReadWrite: access_str = "ComponentAccess::ReadWrite"; break;
    }

    printf("%s(%s, %s)\n", protoid_to_str(c).data(), op_str, access_str);
  };

  if(group_by_access) {
    foreachComponentGroupedByAccess(component_iter_fn);
  } else {
    foreachComponent(component_iter_fn);
  }
}

EntityQuery& EntityQuery::allWithAccess(
    ComponentAccess access, ComponentTypeListPtr all, EntityManagerKey)
{
  return withConditionsByAccess(
      all, { nullptr, 0 }, { nullptr, 0 },
      to_access_mode(access)
  );
}

EntityQuery& EntityQuery::anyWithAccess(
    ComponentAccess access, ComponentTypeListPtr any, EntityManagerKey)
{
  return withConditionsByAccess(
      { nullptr, 0 }, any, { nullptr, 0 },
      to_access_mode(access)
  );
}

EntityQuery& EntityQuery::noneWithAccess(
    ComponentAccess access, ComponentTypeListPtr none, EntityManagerKey)
{
  return withConditionsByAccess(
      { nullptr, 0 }, { nullptr, 0 }, none,
      to_access_mode(access)
  );
}

EntityQuery& EntityQuery::withConditionsByAccess(
    const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none,
    ConditionAccessMode access)
{
  auto for_op = [this,access](const ComponentTypeListPtr& components, ComponentTypeMap *type_map) {
    auto [ component, sz ] = components;
    for(size_t i = 0; i < sz; i++, component++) {
      assertComponentNotAdded(*component);

      type_map->setMut(*component);

      if((unsigned)access & AccessReadOnly) {
        m_access_mask.read.setMut(*component);
      }

      if((unsigned)access & AccessWriteOnly) {
        m_access_mask.write.setMut(*component);
      }
    }
  };

  for_op(all, &m_union_op.all);
  for_op(any, &m_union_op.any);
  for_op(none, &m_union_op.none);

  return *this;
}

EntityQuery::ComponentTypeMap EntityQuery::jointComponentTypeMask() const
{
  return ComponentTypeMap::zero()
    .bitOr(m_union_op.all)
    .bitOr(m_union_op.any)
    .bitOr(m_union_op.none);
}

EntityQuery::ConditionAccessMode EntityQuery::to_access_mode(ComponentAccess access)
{
  switch(access) {
  case ComponentAccess::None:         return AccessNone;
  case ComponentAccess::ReadOnly:     return AccessReadOnly;
  case ComponentAccess::WriteOnly:    return AccessWriteOnly;
  case ComponentAccess::ReadWrite:    return AccessReadWrite;
  }

  return AccessInvalid;   // unreachable (?)
}

ComponentAccess EntityQuery::from_access_mode(ConditionAccessMode access_mode)
{
  switch(access_mode) {
  case AccessNone:         return ComponentAccess::None;
  case AccessReadOnly:     return ComponentAccess::ReadOnly;
  case AccessWriteOnly:    return ComponentAccess::WriteOnly;
  case AccessReadWrite:    return ComponentAccess::ReadWrite;

  default: assert(0);
  }

  return ComponentAccess::None;   // unreachable
}

ComponentProtoId EntityQuery::find_and_clear_lsb(u64 *components)
{
  ComponentProtoId id;
#if __win32
  _BitScanForward64(&id, *components);
#else
  id = __builtin_ffsll(*components)-1;
#endif
  *components &= *components - 1;

  return id;
}

void EntityQuery::assertComponentNotAdded(ComponentProtoId c)
{
  assert(!m_union_op.all.test(c) && !m_union_op.any.test(c) && !m_union_op.none.test(c)
      && !m_access_mask.read.test(c) && !m_access_mask.write.test(c)
      && "a given component can be set only once for a given query!");
}

}
