#pragma once

#include <hm/component.h>

#include <math/geometry.h>
#include <math/quaternion.h>
#include <math/transform.h>

namespace hm {

struct Transform : public Component {
  Transform(u32 entity, const xform::Transform& transform, const AABB& aabb_ = {});
  Transform(u32 entity, vec3 position_, quat orientation_, const AABB& aabb_ = {});
  Transform(u32 entity, vec3 position_, quat orientation_, vec3 scale_, const AABB& aabb_ = {});
  Transform(u32 entity, const mat4& transform, const AABB& aabb_ = {});

  static constexpr Tag tag() { return "Transform"; }

  xform::Transform t;
  AABB aabb;

  Transform& operator=(const xform::Transform& transform);
  Transform& operator=(const mat4& matrix);

  mat4 matrix() const;
  Transform& set(const mat4& matrix);
};

}