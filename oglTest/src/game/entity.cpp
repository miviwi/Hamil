#include <game/entity.h>

namespace game {

Entity::Entity(EntityId id) :
  m_id(id)
{
}

EntityId Entity::id() const
{
  return m_id;
}

}