#include <hm/queryparams.h>
#include <hm/prototype.h>

#include <components.h>

#include <algorithm>

#include <cassert>

namespace hm {

using ComponentGroupMeta = IEntityQueryParams::ComponentGroupMeta;
using RequestedComponent = IEntityQueryParams::RequestedComponent;

const IEntityQueryParams *EntityQueryParams::create_empty()
{
  return new EntityQueryParams();
}

const IEntityQueryParams *EntityQueryParams::create_dbg()
{
  auto params_ptr = create_empty();
  auto& params = *(EntityQueryParams *)params_ptr;

  params.m_group_offsets[0] = 0;
  params.m_group_offsets[1] = 3;
  params.m_group_offsets[2] = 3;
  params.m_group_offsets[3] = 5;

  // ComponentAccess::ReadOnly
  params.m_req.append({
    .kind = AllOf,
    .component = hm::ComponentProto::GameObject,
  });
  params.m_req.append({
      .kind = AllOf,
      .component = hm::ComponentProto::Transform,
  });
  params.m_req.append({
      .kind = AnyOf,
      .component = hm::ComponentProto::Light,
  });

  // ComponentAccess::ReadWrite
  params.m_req.append({
    .kind = AnyOf,
    .component = hm::ComponentProto::Visibility,
  });
  params.m_req.append({
      .kind = AnyOf,
      .component = hm::ComponentProto::Hull,
  });

  return params_ptr;
}

EntityQueryParams::EntityQueryParams() :
  m_group_offsets({ InvalidGroupOffset })
{
  std::fill(m_group_offsets.begin(), m_group_offsets.end(), InvalidGroupOffset);
}

unsigned int EntityQueryParams::numQueryComponents() const
{
  return m_req.size();
}

ComponentGroupMeta EntityQueryParams::componentsForAccess(ComponentAccess access) const
{
  auto group_meta = ComponentGroupMeta {
    .offset = InvalidGroupOffset,
    .length = 0,
  };

  // Sanity check
  assert(access != ComponentAccess::NumAccessTypes &&
      "ComponentAccess::NumAccessTypes is not a valid access type!");

  auto group_idx = (unsigned)access;
  auto group_end = m_group_offsets.at(group_idx);

  // Check if the fetched offset is valid...
  if(group_end == InvalidGroupOffset) return group_meta;

  auto group_head = group_idx > 0 ? m_group_offsets.at(group_idx-1) : 0;
  auto group_len = group_end - group_head;

  group_meta.offset = group_head;
  group_meta.length = group_len;

  return group_meta;
}

RequestedComponent EntityQueryParams::componentByIndex(unsigned index) const
{
  assert(index < m_req.size() && "EntityQueryParams::componentByIndex() index out of range!");

  return m_req.at(index);
}

bool EntityQueryParams::prototypeMatches(const EntityPrototype& prototype, bool exact_match) const
{
  auto remaining_mask = prototype.components();
  for(auto&& [ kind, component ] : m_req) {
    switch(kind) {
    case IEntityQueryParams::AllOf:
      if(!prototype.includes(component)) return false;    // Required component not included!

    case IEntityQueryParams::AnyOf:
      break;      // XXX: no need to do anything here..?

    case IEntityQueryParams::NoneOf:
      if(prototype.includes(component)) return false;     // Includes a forbidden component!

    default: assert(0);   // unreachable
    }

    remaining_mask.clearMut(component);
  }

  if(!exact_match) return true;

  // Check if 'prototype' doesn't have any remaining stray components
  return !remaining_mask.gtZero();
}

}
