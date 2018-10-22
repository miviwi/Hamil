#include <math/transform.h>
#include <math/xform.h>
#include <math/quaternion.h>

namespace xform {

Transform::Transform() :
  m(xform::identity())
{
}

Transform::Transform(const mat4& m_) :
  m(m_)
{
}

Transform& Transform::translate(float x, float y, float z)
{
  m = xform::translate(x, y, z)*m;

  return *this;
}

Transform& Transform::translate(const vec3& pos)
{
  m = xform::translate(pos)*m;

  return *this;
}

Transform& Transform::translate(const vec4& pos)
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

Transform& Transform::scale(const vec3& s)
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

Transform & Transform::rotate(const Quaternion& q)
{
  m = q.toMat4() * m;

  return *this;
}

Transform& Transform::transform(const mat4& t)
{
  m = t*m;

  return *this;
}

const mat4& Transform::matrix() const
{
  return m;
}

vec3 Transform::translation() const
{
  return m.translation();
}

vec3 Transform::scale() const
{
  return m.scale();
}

mat3 Transform::rotation() const
{
  auto r = m.xyz();
  auto s = m.scale().recip();

  r *= mat3::from_rows(s, s, s);

  return r;
}

Quaternion Transform::orientation() const
{
  return Quaternion::from_mat3(rotation());
}

}