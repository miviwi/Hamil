#include <hm/components/transform.h>

namespace hm {

Transform::Transform(u32 entity, const xform::Transform& transform, const AABB& aabb_) :
  Component(entity),
  t(transform), aabb(aabb_)
{
}

Transform::Transform(u32 entity, vec3 position, quat orientation, const AABB& aabb_) :
  Component(entity),
  t(position, orientation), aabb(aabb_)
{
}

Transform::Transform(u32 entity, vec3 position, quat orientation, vec3 scale, const AABB& aabb_) :
  Component(entity),
  t(position, orientation, scale), aabb(aabb_)
{
}

Transform::Transform(u32 entity, const mat4& transform, const AABB& aabb_) :
  Component(entity),
  aabb(aabb_)
{
  set(transform);
}

Transform& Transform::operator=(const xform::Transform& transform)
{
  t = transform;

  return *this;
}

Transform& Transform::operator=(const mat4& matrix)
{
  return set(matrix);
}

mat4 Transform::matrix() const
{
  return t.matrix();
}

Transform& Transform::set(const mat4& matrix)
{
  t = xform::Transform().transform(matrix);

  return *this;
}

}