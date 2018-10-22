#pragma once

#include <hm/component.h>

namespace hm {

struct Model : public Component {
  Model(u32 entity);

  static constexpr Tag tag() { return "Model"; }
};

}