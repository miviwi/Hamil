#pragma once

#include <math/geometry.h>

#pragma pack(push, 1)

struct alignas(16) Quaternion {
  float x, y, z, w;
  
  // Creates an identity Quaternion
  constexpr Quaternion() :
    x(0.0f), y(0.0f), z(0.0f), w(1.0f)
  { }
  constexpr Quaternion(float x_, float y_, float z_, float w_) :
    x(x_), y(y_), z(z_), w(w_)
  { }
  constexpr Quaternion(const float *v) :
    x(v[0]), y(v[1]), z(v[2]), w(v[3])
  { }
  constexpr Quaternion(const vec3& v, float w_) :
    x(v.x), y(v.y), z(v.z), w(w_)
  { }

  Quaternion operator*(float u) const;

  static Quaternion from_axis(vec3 axis, float angle);
  static Quaternion from_euler(float x, float y, float z);

  static Quaternion from_mat3(const mat3& m);
  static Quaternion from_mat4(const mat4& m);

  static Quaternion rotation_between(const vec3& u, const vec3& v);

  static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t);

  mat3 toMat3() const;
  mat4 toMat4() const;

  float length2() const { return dot(*this); }
  float length() const { return sqrtf(length2()); }
  float dot(const Quaternion& b) const { return (x*b.x) + (y*b.y) + (z*b.z) + (w*b.w); }

  Quaternion normalize() const
  {
    auto l = 1.0f / length();
    return (*this) * l;
  }

  Quaternion inverse() const
  {
    return { -x, -y, -z, w };
  }

  Quaternion operator-() const
  {
    return { -x, -y, -z, -w };
  }

  operator float *() { return (float *)this; }
  operator const float *() const { return (const float *)this; }
};

inline Quaternion operator+(const Quaternion& a, const Quaternion& b)
{
  return { a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w };
}

inline Quaternion operator*(const Quaternion& a, const Quaternion& b)
{
#if defined(NO_SSE)
  return {
    a.x*b.w + a.w*b.x + a.z*b.y - a.y*b.z,
    a.y*b.w - a.z*b.x + a.w*b.y - a.x*b.z,
    a.z*b.w - a.y*b.x + a.x*b.y - a.w*b.z,
    a.w*b.w - a.x*b.x + a.y*b.y - a.z*b.z,
  };
#else
  Quaternion c;

  intrin::quat_mult(a, b, c);
  return c;
#endif
}

inline Quaternion Quaternion::operator*(float u) const
{
#if defined(NO_SSE)
  return { x*u, y*u, z*u, w*u };
#else
  Quaternion b;

  intrin::vec4_const_mult(*this, u, b);
  return b;
#endif
}

inline vec3 operator*(const Quaternion& q, const vec3& v)
{
#if defined(NO_SSE)
  vec3 u = { q.x, q.y, q.z };
  float s = q.w;

  return u * 2.0f*u.dot(v)
    + v*(s*s - u.dot(u))
    + u.cross(v) * 2.0f*s;
#else
  alignas(16) vec4 b = { v.x, v.y, v.z, 0.0f };
  alignas(16) vec4 c;

  intrin::quat_vec3_mult(q, b, c);
  return c.xyz();
#endif
}

inline Quaternion& operator*=(Quaternion& a, const Quaternion& b)
{
  a = a*b;
  return a;
}

inline mat3 Quaternion::toMat3() const
{
#if defined(NO_SSE)
  float x2 = x*x,
    y2 = y*y,
    z2 = z*z;

  float xy = x*y,
    xz = x*z,
    xw = x*w,
    yz = y*z,
    yw = y*w,
    zw = z*w;

  return {
    1.0f - 2.0f*y2 - 2.0f*z2, 2.0f*xy - 2.0f*zw,        2.0f*xz + 2.0f*yw,
    2.0f*xy + 2.0f*zw,        1.0f - 2.0f*x2 - 2.0f*z2, 2.0f*yz - 2.0f*xw,
    2.0f*xz - 2.0f*yw,        2.0f*yz + 2.0f*xw,        1.0f - 2.0f*x2 - 2.0f*y2,
  };
#else
  alignas(16) mat4 m;

  intrin::quat_to_mat4x3(*this, m.d);
  return m.xyz();
#endif
}

inline mat4 Quaternion::toMat4() const
{
  return mat4::from_mat3(toMat3());
}

#pragma pack(pop)

using quat = Quaternion;