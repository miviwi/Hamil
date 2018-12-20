#pragma once

#include <hm/component.h>

namespace hm {

struct Hull : public Component {
  Hull(u32 entity);

  static constexpr Tag tag() { return "Hull"; }
};

}
