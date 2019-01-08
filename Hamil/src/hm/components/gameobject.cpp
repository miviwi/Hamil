#include <hm/components/gameobject.h>
#include <hm/entity.h>

#include <cassert>
#include <cstring>
#include <algorithm>

namespace hm {

static constexpr float CompactionThreshold = 0.3f; // More than 30% dead children will cause 
                                                   //   compactChildren() to be invoked

GameObject::GameObject(u32 entity, const std::string& name_, u32 parent_) :
  Component(entity),
  m_parent(parent_)
{
  size_t sz = name_.size() + 1;       // Add space for '\0'
  auto name_str = new char[sz];

  memcpy(name_str, name_.data(), sz); // ...and then copy it over as well
  m_name = name_str;

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
  return m_name;
}

Entity GameObject::parent() const
{
  return Entity(m_parent);
}

void GameObject::foreachChild(std::function<void(Entity)> fn)
{
  int dead_children = 0;
  for(auto& child : m_children) {
    Entity e = child;

    if(!e) {    // Child was previously reaped
      dead_children++;
    } else if(!e.alive()) { // Cache-trashing hell :(
      reapChild(child);
    } else {    // Child is alive
      fn(e);
    }
  }

  auto dead_fraction = (float)dead_children / (float)m_children.size();
  if(dead_fraction > CompactionThreshold) compactChildren();
}

void GameObject::destroyed()
{
  delete[] m_name;

  for(auto child : m_children) {
    Entity e = child;
    e.destroy();
  }
}

GameObjectIterator GameObject::begin()
{
  return GameObjectIterator(&m_children, m_children.begin());
}

GameObjectIterator GameObject::end()
{
  return GameObjectIterator(&m_children, m_children.end());
}

void GameObject::addChild(u32 self)
{
  m_children.append(self);
}

void GameObject::reapChild(u32& child)
{
  child = Entity::Invalid;
}

void GameObject::compactChildren()
{
  m_children.resize(
    std::remove_if(m_children.begin(), m_children.end(),
      [](u32 child) { return child == Entity::Invalid; })
  );
}

GameObjectIterator::GameObjectIterator(Children *children, Children::Iterator it) :
  m(children), m_it(it)
{
}

Entity GameObjectIterator::operator*() const
{
  return Entity(*m_it);
}

GameObjectIterator& GameObjectIterator::operator++()
{
  assert(m_it != m->end() && "Moved GameObjectIterator past end!");

  m_it++;
  while(m_it != m->end()) {
    Entity e = *m_it;
    if(e && e.alive()) break;

    m_it++;  // Skip over dead children
  }

  return *this;
}

GameObjectIterator GameObjectIterator::operator++(int)
{
  auto self = *this;
  ++*this;

  return self;
}

bool GameObjectIterator::operator==(const GameObjectIterator& other) const
{
  return m_it == other.m_it;
}

bool GameObjectIterator::operator!=(const GameObjectIterator& other) const
{
  return m_it != other.m_it;
}

bool GameObjectIterator::atEnd() const
{
  return m_it == m->end();
}

}