#pragma once

#include <hm/component.h>

#include <string>
#include <string_view>
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
  std::string_view m_name;

  u32 m_parent;
  std::vector<u32> m_children;

  void addChild(u32 self);
  void reapChild(u32& child);
};

}