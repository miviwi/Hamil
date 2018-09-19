#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/gameobject.h>

namespace hm {

Component::Component(Entity e) :
  m_entity(e)
{
}

Entity Component::entity() const
{
  return m_entity;
}

GameObject& Component::gameObject() const
{
  return entity().component<GameObject>().get();
}

Component::operator bool() const
{
  return entity() && entity().alive();
}

void Component::destroyed()
{
}

}