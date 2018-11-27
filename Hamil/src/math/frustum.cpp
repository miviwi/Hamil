#include <math/frustum.h>

frustum3::frustum3(const vec3& eye, mat4 vp)
{
  vec4 r = vp.row(3) - vp.row(0);
  vec4 l = vp.row(3) + vp.row(0);
  vec4 t = vp.row(3) - vp.row(1);
  vec4 b = vp.row(3) + vp.row(1);
  vec4 f = vp.row(3) - vp.row(2);
  vec4 n = vp.row(3) + vp.row(2);

  planes[0] = t;
  planes[1] = l;
  planes[2] = b;
  planes[3] = r;
  planes[4] = n;
  planes[5] = f;

  for(auto& plane : planes) plane = plane.normalize();
}

frustum3::frustum3(const vec3& eye, const mat4& view, const mat4& projection) :
  frustum3(eye, projection * view)
{
}

bool frustum3::sphereInside(const vec3& pos, float r) const
{
  vec4 p(pos, 1.0f);
  for(const auto& plane : planes) {
    if(plane.dot(p) <= -r) return false;
  } 

  return true;
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
