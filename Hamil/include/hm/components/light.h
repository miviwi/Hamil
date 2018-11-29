#pragma once

#include <hm/component.h>

#include <math/geometry.h>

namespace hm {

// TODO: temporary
struct Light : public Component {
  Light(u32 entity);

  static Tag tag() { return "Light"; }

  enum LightType {
    Directional, Spot, Point,
  };

  LightType type;

  vec3 v;
  vec3 color;

  bool shadow_casting = false;
};

}