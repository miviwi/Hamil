#include <hm/components/transform.h>

namespace hm {

Transform::Transform(u32 entity, const xform::Transform& transform) :
  Component(entity),
  t(transform)
{
}

Transform::Transform(u32 entity, vec3 position, quat orientation) :
  Component(entity)
{
  t
    .rotate(orientation)
    .translate(position);
}

Transform::Transform(u32 entity, vec3 position, quat orientation, vec3 scale) :
  Component(entity)
{
  t
    .rotate(orientation)
    .scale(scale)
    .translate(position);
}

Transform::Transform(u32 entity, const mat4& transform) :
  Component(entity)
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