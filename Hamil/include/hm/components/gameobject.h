#pragma once

#include <hm/component.h>

#include <string>

namespace hm {

// !$Component
struct GameObject : public Component {
  GameObject(u32 entity, const std::string& name_) :
    Component(entity),
    m_name(name_)
  {
  }

  const std::string& name() const;

private:
  std::string m_name;
};

}