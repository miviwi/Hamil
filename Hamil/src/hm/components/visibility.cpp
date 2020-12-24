#include <hm/components/visibility.h>

namespace hm {

Visibility::Visibility(u32 entity) :
  Component()
{
}

ek::VisibilityObject *Visibility::visObject()
{
  return &vis;
}

const ek::VisibilityObject *Visibility::visObject() const
{
  return &vis;
}

}