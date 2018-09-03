#pragma once

#include <game/component.h>

namespace game {

// !$Component
struct TestComponent : public Component {
  TestComponent(u32 entity, const char *name_) :
    Component(entity),
    name(name_)
  {
  }

  const char *name;
};


}