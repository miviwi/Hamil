#pragma once

#include <hm/component.h>

namespace hm {

struct Hull : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */
  >;

  Hull(u32 entity);

  static const Tag tag() { return "Hull"; }

  void *hull;
};

}
