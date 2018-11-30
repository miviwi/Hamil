#pragma once

#include <math/geometry.h>

#include <array>

// Helper class for performing frustum culling
struct frustum3 {
  // - When far=inf inf_far must be 'true' (TODO!)
  frustum3(mat4 vp, bool inf_far = false);
  // Constructs a frustum by multiplying 'view' and 'projection'
  //   - When far=inf inf_far must be 'true' (TODO!)
  frustum3(const mat4& view, const mat4& projection, bool inf_far = false);

  std::array<vec4, 8> corners;  // See frustum.cpp:7
  std::array<vec4, 6> planes;   // t, l, b, r, n, f

  // 'pos' must be in world space
  bool sphereInside(const vec3& pos, float r) const;
  bool sphereInside(const Sphere& s) const;
  // The points of the AABB must be in world space
  bool aabbInside(const AABB& aabb) const;
};