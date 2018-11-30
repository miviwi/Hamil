#include <math/frustum.h>

frustum3::frustum3(mat4 vp, bool inf_far)
{
  mat4 inv_vp = vp.inverse();

  //                         7--------6
  //                        /|       /|
  //     ^ Y               / |      / |
  //     | _              3--------2  |
  //     | /' Z           |  |     |  |
  //     |/               |  5-----|--4
  //     + ---> X         | /      | /
  //                      |/       |/
  //                      1--------0
  corners = { {
    {  1.0f, -1.0f, -1.0f, 1.0f },
    { -1.0f, -1.0f, -1.0f, 1.0f },
    {  1.0f,  1.0f, -1.0f, 1.0f },
    { -1.0f,  1.0f, -1.0f, 1.0f },
    {  1.0f, -1.0f,  1.0f, 1.0f },
    { -1.0f, -1.0f,  1.0f, 1.0f },
    {  1.0f,  1.0f,  1.0f, 1.0f },
    { -1.0f,  1.0f,  1.0f, 1.0f },
  } };

  // Unproject the NDC cube into a world-space frustum
  for(auto& c : corners) c = (inv_vp * c).perspectiveDivide();

  auto plane_from_points = [](vec4 p0, vec4 p1, vec4 p2) -> vec4 {
    vec3 v = p1 - p0;
    vec3 u = p2 - p0;

    vec3 n = u.cross(v).normalize();
    float d = n.dot(p0.xyz());

    return vec4(n, -d);
  };

  vec4 r = plane_from_points(corners[0], corners[4], corners[2]);
  vec4 l = plane_from_points(corners[1], corners[3], corners[5]);
  vec4 t = plane_from_points(corners[3], corners[2], corners[7]);
  vec4 b = plane_from_points(corners[1], corners[5], corners[0]);
  vec4 n = plane_from_points(corners[1], corners[0], corners[3]);
  vec4 f = plane_from_points(corners[5], corners[7], corners[4]);

  planes[0] = t;
  planes[1] = l;
  planes[2] = b;
  planes[3] = r;
  planes[4] = n;
  planes[5] = !inf_far ? f : n;  // Avoid NaNs when far=inf
}

frustum3::frustum3(const mat4& view, const mat4& projection, bool inf_far) :
  frustum3(projection * view, inf_far)
{
}

bool frustum3::sphereInside(const vec3& pos, float r) const
{
  vec4 p(pos, 1.0f);
  for(const auto& plane : planes) {
    if(plane.dot(p) < -r) return false;
  } 

  return true;
}

bool frustum3::sphereInside(const Sphere& s) const
{
  return sphereInside(s.c, s.r);
}

bool frustum3::aabbInside(const AABB& aabb) const
{
  std::array<vec4, 8> points = {
    vec4(aabb.min.x, aabb.min.y, aabb.min.z, 1.0f),
    vec4(aabb.max.x, aabb.min.y, aabb.min.z, 1.0f),
    vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.0f),
    vec4(aabb.min.x, aabb.min.y, aabb.max.z, 1.0f),
    vec4(aabb.max.x, aabb.max.y, aabb.min.z, 1.0f),
    vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.0f),
    vec4(aabb.min.x, aabb.max.y, aabb.max.z, 1.0f),
    vec4(aabb.max.x, aabb.max.y, aabb.max.z, 1.0f),
  };

  for(const auto& plane : planes) {
    int outside = 0;
    for(const auto& p : points) {
      if(plane.dot(p) > 0.0f) break;

      outside++;
    }

    if(outside == points.size()) return false;
  }

  return true;
}
