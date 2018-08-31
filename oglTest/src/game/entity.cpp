#include <game/entity.h>

namespace game {

EntityId EntityManager::newId()
{
  return m_next_id.next();
}

Entity::Entity(EntityId id) :
  m_id(id)
{
}

}