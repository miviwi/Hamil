#pragma once

#include <hm/component.h>

#include <util/smallvector.h>

#include <string>
#include <vector>
#include <functional>
#include <iterator>

namespace hm {

class Entity;

struct GameObjectIterator :
  std::iterator<
    std::forward_iterator_tag,
    Entity
  > {

  using Children = util::SmallVector<u32, 16>;

  GameObjectIterator(Children *children, Children::Iterator it);

  Entity operator*() const;

  GameObjectIterator& operator++();
  GameObjectIterator operator++(int);

  bool operator==(const GameObjectIterator& other) const;
  bool operator!=(const GameObjectIterator& other) const;

  bool atEnd() const;

private:
  Children *m;
  Children::Iterator m_it;
};

struct GameObject : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */, const std::string& /* name */, u32 /* parent */
  >;

  GameObject(u32 entity, const std::string& name_, u32 parent);
  GameObject(u32 entity, const std::string& name_);

  static const Tag tag() { return "GameObject"; }

  const char *name() const;

  Entity parent() const;
  void foreachChild(std::function<void(Entity)> fn);

  void destroyed();

  GameObjectIterator begin();
  GameObjectIterator end();

private:
  // The order of the members is very deliberate and avoids
  //   alignment padding

  u32 m_parent;
  util::SmallVector<u32, 16> m_children;

  const char *m_name;

  void addChild(u32 self);
  void reapChild(u32& child);

  void compactChildren();
};

}
