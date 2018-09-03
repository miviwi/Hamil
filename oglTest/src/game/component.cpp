#include <game/component.h>

namespace game {

EntityId Component::entity() const
{
  return m_entity;
}

}