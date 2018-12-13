#pragma once

#include <hm/component.h>

#include <math/geometry.h>

namespace hm {

// TODO: temporary
struct Light : public Component {
  Light(u32 entity);

  static Tag tag() { return "Light"; }

  enum Type : i16 {
    Invalid,

    // The Entities position == direction
    Directional,

    Spot,
    Line, Rectangle, Quad, Sphere, Disk,
    ImageBasedLight,
  };

  enum Flags : u16 {
    Default = 0,
    ShadowCasting = 1<<0,
  };

  Type type = Invalid;
  u16 flags = Default;

  // The Light's falloff radius
  float radius;

  // Components can be > 1.0f
  vec3 color;

  union {
    struct {
      vec3 direction;
      float angle;
    } spot;

    struct {
      float half_extent;
    } line;

    struct {
      float radius;
    } sphere;
  };
};

}