#include <hm/entity.h>
#include <hm/entityman.h>

namespace hm {

Entity::Entity(EntityId id) :
  m_id(id)
{
}

EntityId Entity::id() const
{
  return m_id;
}

Entity::operator bool() const
{
  return id() != Invalid;
}

bool Entity::alive() const
{
  return entities().alive(id());
}

}