#pragma once

#include <hm/component.h>

#include <util/smallvector.h>

#include <string>
#include <vector>
#include <functional>

namespace hm {

class Entity;

struct GameObject : public Component {
  GameObject(u32 entity, const std::string& name_, u32 parent);
  GameObject(u32 entity, const std::string& name_);

  const char *name() const;

  Entity parent() const;
  void foreachChild(std::function<void(Entity)> fn);

  void destroyed();

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

constexpr int z = sizeof(util::SmallVector<u32>);
constexpr int x = sizeof(GameObject);

}