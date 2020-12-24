#pragma once

#include <hm/component.h>

#include <math/geometry.h>
#include <gx/resourcepool.h>

namespace hm {

// TODO: temporary
struct Material : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */
  >;

  Material(u32 entity);

  static Tag tag() { return "Material"; }

  enum DiffuseType : u32 {
    DiffuseConstant, DiffuseTexture,

    Other = 1u << (u32)(sizeof(u32)*CHAR_BIT - 1),
  };

  bool unshaded = false;

  DiffuseType diff_type;
  union {
    vec3 diff_color;

    struct {
      gx::ResourcePool::Id id;
      gx::ResourcePool::Id sampler_id;
    } diff_tex;
  };

  float metalness;
  float roughness;
  vec3 ior;
};

}
