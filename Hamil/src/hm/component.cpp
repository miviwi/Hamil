#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/gameobject.h>

namespace hm {

Component::Component()
{
}

Component::operator bool() const
{
  /*
  return entity() && entity().alive();
  */
  return false;
}

void Component::destroyed()
{
}

}
