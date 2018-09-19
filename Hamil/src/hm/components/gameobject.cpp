#include <hm/components/gameobject.h>
#include <hm/entity.h>

#include <cstring>

namespace hm {

GameObject::GameObject(u32 entity, const std::string& name_, u32 parent_) :
  Component(entity),
  m_parent(parent_)
{
  size_t sz = name_.size() + 1;       // Add space for '\0'
  auto name_str = new char[sz];

  memcpy(name_str, name_.data(), sz); // ...and then copy it over as well
  m_name = { name_str, sz-1 };

  if(parent_ != Entity::Invalid) {
    parent().gameObject().addChild(entity);
  }
}

GameObject::GameObject(u32 entity, const std::string& name_) :
  GameObject(entity, name_, Entity::Invalid)
{
}

const char *GameObject::name() const
{
  return m_name.data();
}

Entity GameObject::parent() const
{
  return Entity(m_parent);
}

void GameObject::foreachChild(std::function<void(Entity)> fn)
{
  for(auto& child : m_children) {
    Entity e = child;
    if(!e) continue;  // Child was previously reaped

    if(!e.alive()) {
      reapChild(child);
      continue;
    }

    fn(e);
  }
}

void GameObject::destroyed()
{
  delete m_name.data();

  for(auto child : m_children) {
    Entity e = child;
    e.destroy();
  }
}

void GameObject::addChild(u32 self)
{
  m_children.push_back(self);
}

void GameObject::reapChild(u32& child)
{
  child = Entity::Invalid;
}

}