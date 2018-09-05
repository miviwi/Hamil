#pragma once

#include <math/geometry.h>

#pragma pack(push, 1)

struct alignas(16) Quaternion {
  float x, y, z, w;

  constexpr Quaternion() :
    x(0.0f), y(0.0f), z(0.0f), w(1.0f)
  { }
  constexpr Quaternion(float x_, float y_, float z_, float w_) :
    x(x_), y(y_), z(z_), w(w_)
  { }
  constexpr Quaternion(const float *v) :
    x(v[0]), y(v[1]), z(v[2]), w(v[3])
  { }

  Quaternion operator*(float u) const;

  static Quaternion from_axis(vec3 axis, float angle)
  {
    float half_angle = angle / 2.0f;
    float s = sinf(half_angle);

    axis *= s;

    return { axis.x, axis.y, axis.z, cosf(half_angle) };
  }

  static Quaternion from_euler(float x, float y, float z)
  {
    float cx = cos(x / 2.0f),
      cy = cos(y / 2.0f),
      cz = cos(z / 2.0f);

    float sx = sin(x / 2.0f),
      sy = sin(y / 2.0f),
      sz = sin(z / 2.0f);

    float cycz = cy*cz,
      sysz = sy*sz,
      cysz = cy*sz,
      sycz = sy*cz;

    Quaternion q = {
      sx*cycz - cx*sysz,
      cx*sycz + sx*cysz,
      cx*cysz - sx*sycz,
      cx*cycz + sx*sysz
    };

    return q.normalize();
  }

  static Quaternion from_mat3(const Matrix3<float>& m);
  static Quaternion from_mat4(const Matrix4<float>& m);

  Matrix3<float> to_mat3() const;
  Matrix4<float> to_mat4() const;

  float length() const { return sqrtf(x*x + y*y + z*z + w*w); }
  float dot(const Quaternion& b) const { return (x*b.x) + (y*b.y) + (z*b.z) + (w*b.w); }

  Quaternion normalize() const
  {
    auto l = 1.0f / length();
    return (*this) * l;
  }

  inline Quaternion cross(const Quaternion& b) const;

  operator float *() { return (float *)this; }
  operator const float *() const { return (const float *)this; }
};

inline Quaternion operator*(const Quaternion& a, const Quaternion& b)
{
  return {
    a.x*b.w + a.w*b.x + a.z*b.y - a.y*b.z,
    a.y*b.w - a.z*b.x + a.w*b.y - a.x*b.z,
    a.z*b.w - a.y*b.x + a.x*b.y - a.w*b.z,
    a.w*b.w - a.x*b.x + a.y*b.y - a.z*b.z,
  };
}

inline Quaternion Quaternion::operator*(float u) const
{
  Quaternion b;

  intrin::vec4_const_mult(*this, u, b);
  return b;
}

inline vec3 operator*(const Quaternion& q, const vec3& v)
{
  vec3 u ={ q.x, q.y, q.z };
  float s = q.w;

  return u * 2.0f*u.dot(v)
    + v*(s*s - u.dot(u))
    + u.cross(v) * 2.0f*s;
}

inline Quaternion Quaternion::cross(const Quaternion& b) const
{
  Quaternion c;

  intrin::quat_cross((const float *)this, b, c);
  return c;
}

inline Quaternion& operator*=(Quaternion& a, const Quaternion& b)
{
  a = a*b;
  return a;
}

inline mat3 Quaternion::to_mat3() const
{
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
    1.0f - 2.0f*y2 - 2.0f*z2,        2.0f*xy - 2.0f*zw,        2.0f*xz + 2.0f*yw,
    2.0f*xy + 2.0f*zw, 1.0f - 2.0f*x2 - 2.0f*z2,        2.0f*yz - 2.0f*xw,
    2.0f*xz - 2.0f*yw,        2.0f*yz + 2.0f*xw, 1.0f - 2.0f*x2 - 2.0f*y2,
  };
}

inline mat4 Quaternion::to_mat4() const
{
  return mat4::from_mat3(to_mat3());
}

// Source: https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
inline Quaternion Quaternion::from_mat3(const mat3& m)
{
  Quaternion q;
  float t;

  if(m(2, 2) < 0.0f) {
    if(m(0, 0) > m(1, 1)) {
      t = 1.0f + m(0, 0) - m(1, 1) - m(2, 2);
      q = { t, m(1, 0)+m(0, 1), m(0, 2)+m(2, 0), m(2, 1)-m(1, 2) };
    } else {
      t = 1.0f - m(0, 0) + m(1, 1) - m(2, 2);
      q = { m(1, 0)+m(0, 1), t, m(2, 1)+m(1, 2), m(0, 2)-m(2, 0) };
    }
  } else {
    if(m(0, 0) < -m(1, 1)) {
      t = 1.0f - m(0, 0) - m(1, 1) + m(2, 2);
      q = { m(0, 2)+m(2, 0), m(2, 1)+m(1, 2), t, m(1, 0)-m(0, 1) };
    } else {
      t = 1.0f + m(0, 0) + m(1, 1) + m(2, 2);
      q = { m(2, 1)-m(1, 2), m(0, 2)-m(2, 0), m(1, 0)-m(0, 1), t };
    }
  }

  t = 0.5f / sqrtf(t);

  return q * t;
}

inline Quaternion Quaternion::from_mat4(const mat4& m)
{
  return from_mat3(m.xyz());
}

#pragma pack(pop)