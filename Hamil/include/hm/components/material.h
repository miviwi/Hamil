#pragma once

#include <hm/component.h>

#include <math/geometry.h>
#include <gx/resourcepool.h>

namespace hm {

// TODO: temporary
struct Material : public Component {
  Material(u32 entity);

  static Tag tag() { return "Material"; }

  enum DiffuseType {
    DiffuseConstant, DiffuseTexture,
  };

  bool unshaded = false;

  DiffuseType diff_type;
  vec3 diff_color;
  gx::ResourcePool::Id diff_tex_id;

  float metalness;
  float roughness;
  vec3 ior;
};

}