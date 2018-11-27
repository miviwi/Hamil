#pragma once

#include <math/geometry.h>

#include <array>

// Helper class for performing frustum culling
struct frustum3 {
  frustum3(const vec3& eye, mat4 vp);
  // Constructs a frustum by multiplying 'view' and 'projection'
  frustum3(const vec3& eye, const mat4& view, const mat4& projection);

  std::array<vec3, 8> corners;
  std::array<vec4, 6> planes;   // t, l, b, r, n, f

  // 'pos' must be in VIEW space
  bool sphereInside(const vec3& pos, float r) const;
  // The points of the AABB must be in VIEW space
  bool aabbInside(const AABB& aabb) const;
};