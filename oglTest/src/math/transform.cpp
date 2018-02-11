#include <math/transform.h>

namespace xform {


Transform::Transform() :
  m(xform::identity())
{
}

Transform& Transform::translate(float x, float y, float z)
{
  m = xform::translate(x, y, z)*m;

  return *this;
}

Transform& Transform::translate(vec3 pos)
{
  m = xform::translate(pos)*m;

  return *this;
}

Transform& Transform::translate(vec4 pos)
{
  m = xform::translate(pos)*m;

  return *this;
}

Transform& Transform::scale(float x, float y, float z)
{
  m = xform::scale(x, y, z)*m;

  return *this;
}

Transform& Transform::scale(float s)
{
  m = xform::scale(s)*m;

  return *this;
}

Transform& Transform::scale(vec3 s)
{
  m = xform::scale(s.x, s.y, s.z)*m;

  return *this;
}

Transform& Transform::rotx(float angle)
{
  m = xform::rotx(angle)*m;

  return *this;
}

Transform& Transform::roty(float angle)
{
  m = xform::roty(angle)*m;

  return *this;
}

Transform& Transform::rotz(float angle)
{
  m = xform::rotz(angle)*m;

  return *this;
}

Transform& Transform::transform(const mat4 & t)
{
  m = t*m;

  return *this;
}

const mat4& Transform::matrix() const
{
  return m;
}

}