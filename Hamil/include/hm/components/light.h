#pragma once

#include <hm/component.h>

#include <math/geometry.h>

#include <tuple>

namespace hm {

// TODO: temporary
struct Light : public Component {
  using ConstructorParamPack = std::tuple<u32>;

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
      vec3 tangent;
      float length;
    } line;

    struct {
      float radius;
    } sphere;
  };
};

}
