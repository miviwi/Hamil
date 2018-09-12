#include <hm/component.h>

namespace hm {

Component::Component(Entity e) :
  m_entity(e)
{
}

Entity Component::entity() const
{
  return m_entity;
}

Component::operator bool() const
{
  return entity() && entity().alive();
}

void Component::destroyed()
{
}

}