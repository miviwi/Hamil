#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/components/gameobject.h>

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

void Entity::destroy()
{
  entities().destroyEntity(id());
}

GameObject& Entity::gameObject()
{
  return *component<GameObject>().ptr();
}

}