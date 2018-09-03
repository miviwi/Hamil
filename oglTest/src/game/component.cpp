#include <game/component.h>

namespace game {

Component::Component(Entity e) :
  m_entity(e)
{
}

Entity Component::entity() const
{
  return m_entity;
}

}