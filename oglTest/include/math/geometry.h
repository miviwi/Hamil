#pragma once

#include <math/intrin.h>

#include <cmath>
#include <algorithm>
#include <limits>
#include <type_traits>

static unsigned pow2_round(unsigned v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;

  return v;
}

constexpr double PI = 3.1415926535897932384626433832795;
constexpr float PIf = (float)PI;

#pragma pack(push, 1)

template <typename T>
struct Vector2 {
  constexpr Vector2(T x_, T y_) :
    x(x_), y(y_)
  { }
  Vector2(const T *v) :
    x(v[0]), y(v[1])
  {
  }
  Vector2() :
    x(0), y(0)
  { }

  union {
    struct { T x, y; };
    struct { T s, t; };
  };

  T length() const { return (T)sqrt((x*x) + (y*y)); }
  T dot(const Vector2& b) const { return (x*b.x) + (y*b.y); }

  Vector2 normalize() const
  {
    T l = length();
    return Vector2{ x/l, y/l };
  }

  T distance2(const Vector2& v) const
  {
    Vector2 d = v - *this;

    return d.x*d.x + d.y*d.y;
  }
  T distance(const Vector2& v) const
  {
    return sqrt(distance2(v));
  };

  Vector2 recip() const { return { (T)1 / x, (T)1 / y }; }

  template <typename U>
  Vector2<U> cast() const
  {
    return Vector2<U>{ (U)x, (U)y };
  }

  operator float *() { return (float *)this; }
  operator const float *() const { return (float *)this; }
};

template <typename T>
Vector2<T> operator+(Vector2<T> a, Vector2<T> b)
{
  return Vector2<T>{ a.x+b.x, a.y+b.y };
}

template <typename T>
Vector2<T> operator-(Vector2<T> a, Vector2<T> b)
{
  return Vector2<T>{ a.x-b.x, a.y-b.y };
}

template <typename T>
Vector2<T> operator*(Vector2<T> a, Vector2<T> b)
{
  return Vector2<T>{ a.x*b.x, a.y*b.y };
}

template <typename T>
Vector2<T> operator*(Vector2<T> a, T u)
{
  return Vector2<T>{ a.x*u, a.y*u };
}

template <typename T>
Vector2<T>& operator+=(Vector2<T>& a, Vector2<T> b)
{
  a = a+b;
  return a;
}

template <typename T>
Vector2<T>& operator*=(Vector2<T>& a, Vector2<T>& b)
{
  a = a*b;
  return a;
}

template <typename T>
Vector2<T>& operator*=(Vector2<T>& a, T u)
{
  a = a*u;
  return a;
}

template <typename T>
bool operator==(Vector2<T> a, Vector2<T> b)
{
  return a.x == b.x && a.y == b.y;
}

template <typename T>
bool operator!=(Vector2<T> a, Vector2<T> b)
{
  return a.x != b.x && a.y != b.y;
}

template <typename T>
Vector2<T> line_normal(Vector2<T> a, Vector2<T> b)
{
  return Vector2<T>{ -(b.y - a.y), b.x - a.x }.normalize();
}

using vec2  = Vector2<float>;
using ivec2 = Vector2<int>;
using uvec2 = Vector2<unsigned>;

template <typename T>
struct Vector3 {
  constexpr Vector3(T x_, T y_, T z_) :
    x(x_), y(y_), z(z_)
  { }
  constexpr Vector3(T v) :
    x(v), y(v), z(v)
  { }
  Vector3(const T *v) :
    x(v[0]), y(v[1]), z(v[2])
  { }
  Vector3() :
    x(0), y(0), z(0)
  { }

  union {
    struct { T x, y, z; };
    struct { T r, g, b; };
    struct { T s, t, p; };
  };

  Vector2<T> xy() const { return Vector2<T>{ x, y }; }

  T length() const { return (T)sqrt(x*x + y*y + z*z); }
  T dot(const Vector3& b) const { return x*b.x + y*b.y + z*b.z; }

  Vector3 normalize() const
  {
    T l = (T)1 / length();
    return (*this) * l;
  }

  Vector3 cross(const Vector3& b) const
  {
    return Vector3{ y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x };
  }

  T distance2(const Vector3& v) const
  {
    Vector3 d = v - *this;

    return d.x*d.x + d.y*d.y + d.z*d.z;
  }
  T distance(const Vector3& v) const
  {
    return sqrt(distance2(v));
  }

  Vector3 recip() const { return { (T)1 / x, (T)1 / y, (T)1 / z }; }

  operator float *() { return (float *)this; }
  operator const float *() const { return (float *)this; }
};

template <typename T>
Vector3<T> operator+(Vector3<T> a, Vector3<T> b)
{
  return Vector3<T>{ a.x+b.x, a.y+b.y, a.z+b.z };
}

template <typename T>
Vector3<T> operator-(Vector3<T> a, Vector3<T> b)
{
  return Vector3<T>{ a.x-b.x, a.y-b.y, a.z-b.z };
}

template <typename T>
Vector3<T> operator*(Vector3<T> a, Vector3<T> b)
{
  return Vector3<T>{ a.x*b.x, a.y*b.y, a.z*b.z };
}

template <typename T>
Vector3<T> operator*(Vector3<T> a, T u)
{
  return Vector3<T>{ a.x*u, a.y*u, a.z*u };
}

template <typename T>
Vector3<T>& operator+=(Vector3<T>& a, Vector3<T> b)
{
  a = a+b;
  return a;
}

template <typename T>
Vector3<T>& operator-=(Vector3<T>& a, Vector3<T> b)
{
  a = a-b;
  return a;
}

template <typename T>
Vector3<T> operator-(Vector3<T> v)
{
  return Vector3<T>{ -v.x, -v.y, -v.z };
}

template <typename T>
Vector3<T>& operator*=(Vector3<T>& a, T u)
{
  a = a*u;
  return a;
}

template <typename T>
bool operator==(Vector3<T> a, Vector3<T> b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <typename T>
bool operator!=(Vector3<T> a, Vector3<T> b)
{
  return a.x != b.x && a.y != b.y && a.z != b.z;
}

using vec3  = Vector3<float>;
using ivec3 = Vector3<int>;
using uvec3 = Vector3<unsigned>;

inline vec3 vec3::cross(const vec3& v) const
{
  alignas(16) float a[4] = { x, y, z, 0 };
  alignas(16) float b[4] = { v.x, v.y, v.z, 0 };
  alignas(16) float c[4];

  intrin::vec3_cross(a, b, c);
  return vec3{ c[0], c[1], c[2] };
}

template <typename T>
struct Vector4 {
  constexpr Vector4(T x_, T y_, T z_, T w_) :
    x(x_), y(y_), z(z_), w(w_)
  { }
  Vector4(Vector2<T> xy, T z_, T w_) :
    x(xy.x), y(xy.y), z(z_), w(w_)
  { }
  Vector4(Vector2<T> xy, T z_) :
    x(xy.x), y(xy.y), z(z_), w(1)
  { }
  Vector4(Vector2<T> xy) :
    x(xy.x), y(xy.y), z(0), w(1)
  { }
  Vector4(Vector3<T> xyz) :
    x(xyz.x), y(xyz.y), z(xyz.z), w(1.0f)
  { }
  Vector4(Vector3<T> xyz, T w_) :
    x(xyz.x), y(xyz.y), z(xyz.z), w(w_)
  { }
  Vector4(const T *v) :
    x(v[0]), y(v[1]), z(v[2]), w(v[3])
  { }
  Vector4() :
    x(0), y(0), z(0), w(1)
  { }

  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    struct { T s, t, p, q; };
  };

  Vector3<T> xyz() const { return Vector3<T>{ x, y, z }; }

  T length() const { return (T)sqrt(x*x + y*y + z*z + w*w); }
  T dot(const Vector4& b) const { return x*b.x + y*b.y + z*b.z + w*b.w; }

  Vector4 recip() const { return { (T)1 / x, (T)1 / y, (T)1 / z, (T) / w }; }

  operator float *() { return (float *)this; }
  operator const float *() const { return (float *)this; }
};

template <typename T>
Vector4<T> operator+(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ (T)(a.x+b.x), (T)(a.y+b.y), (T)(a.z+b.z), (T)(a.w+b.w) };
}

template <typename T>
Vector4<T> operator-(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ (T)(a.x-b.x), (T)(a.y-b.y), (T)(a.z-b.z), (T)(a.w-b.w) };
}

template <typename T>
Vector4<T> operator*(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w };
}

template <typename T>
Vector4<T> operator*(Vector4<T> a, T u)
{
  return Vector4<T>{ a.x*u, a.y*u, a.z*u, a.w*u };
}

template <typename T>
Vector4<T>& operator+=(Vector4<T>& a, Vector4<T> b)
{
  a = a+b;
  return a;
}

template <typename T>
Vector4<T>& operator*=(Vector4<T>& a, T u)
{
  a = a*u;
  return a;
}

template <typename T>
bool operator==(Vector4<T> a, Vector4<T> b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template <typename T>
bool operator!=(Vector4<T> a, Vector4<T> b)
{
  return a.x != b.x && a.y != b.y && a.z != b.z && a.w != b.w;
}

using vec4  = Vector4<float>;
using ivec4 = Vector4<int>;
using uvec4 = Vector4<unsigned>;

inline vec4 operator*(vec4 a, float u)
{
  alignas(16) vec4 b;

  intrin::vec4_const_mult(a, u, b);
  return b;
}

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
  Quaternion c;

  intrin::quat_mult(a, b, c);
  return c;
}

inline Quaternion Quaternion::operator*(float u) const
{
  Quaternion b;

  intrin::vec4_const_mult(*this, u, b);
  return b;
}

inline Quaternion Quaternion::cross(const Quaternion& b) const
{
  Quaternion c;

  intrin::quat_cross((const float *)this, b, c);
  return c;
}

template <typename T>
struct Matrix2 {
  T d[2*2];

  Matrix2& operator *=(Matrix2& b) { *this = *this * b; return *this; }

  operator float *() { return d; }
  operator const float *() const { return d; }
};

template <typename T>
Matrix2<T> operator*(Matrix2<T> a, Matrix2<T> b)
{
  return Matrix2<T>{
      a.d[0]*b.d[0]+a.d[1]*b.d[2], a.d[0]*b.d[1]+a.d[1]*b.d[3],
      a.d[2]*b.d[0]+a.d[3]*b.d[2], a.d[2]*b.d[1]+a.d[3]*b.d[3],
  };
}

using mat2  = Matrix2<float>;
using imat2 = Matrix2<int>;

template <typename T>
struct Matrix3 {
  T d[3*3];

  Matrix3& operator *=(Matrix3& b) { *this = *this * b; return *this; }

  operator float *() { return d; }
  operator const float *() const { return d; }
};

template <typename T>
Matrix3<T> operator*(Matrix3<T> a, Matrix3<T> b)
{
  return Matrix3<T>{
      a.d[0]*b.d[0]+a.d[1]*b.d[3]+a.d[2]*b.d[6], a.d[0]*b.d[1]+a.d[1]*b.d[4]+a.d[2]*b.d[7], a.d[0]*b.d[2]+a.d[1]*b.d[5]+a.d[2]*b.d[8],
      a.d[3]*b.d[0]+a.d[4]*b.d[3]+a.d[5]*b.d[6], a.d[3]*b.d[1]+a.d[4]*b.d[4]+a.d[5]*b.d[7], a.d[3]*b.d[2]+a.d[4]*b.d[5]+a.d[5]*b.d[8],
      a.d[6]*b.d[0]+a.d[7]*b.d[3]+a.d[8]*b.d[6], a.d[6]*b.d[1]+a.d[7]*b.d[4]+a.d[8]*b.d[7], a.d[6]*b.d[2]+a.d[7]*b.d[5]+a.d[8]*b.d[8],
  };
}

using mat3  = Matrix3<float>;
using imat3 = Matrix3<int>;

template <typename T>
struct Matrix4 {
  alignas(16) T d[4*4];

  Matrix4& operator *=(Matrix4& b) { *this = *this * b; return *this; }

  Matrix4 transpose() const
  {
    return Matrix4{
      d[0], d[4], d[8],  d[12],
      d[1], d[5], d[9],  d[13],
      d[2], d[6], d[10], d[14],
      d[3], d[7], d[11], d[15]
    };
  }
  // Must be non-singular!
  inline Matrix4 inverse() const;

  Matrix3<T> xyz() const
  {
    return Matrix3<T>{
      d[0], d[1], d[2],
      d[4], d[5], d[6],
      d[8], d[9], d[10]
    };
  }

  operator float *() { return d; }
  operator const float *() const { return d; }
};

template <typename T>
Matrix4<T> operator*(Matrix4<T> a, Matrix4<T> b)
{
  return Matrix4<T>{
      a.d[ 0]*b.d[ 0]+a.d[ 1]*b.d[ 4]+a.d[ 2]*b.d[ 8]+a.d[ 3]*b.d[12], a.d[ 0]*b.d[ 1]+a.d[ 1]*b.d[ 5]+a.d[ 2]*b.d[ 9]+a.d[ 3]*b.d[13], a.d[ 0]*b.d[2 ]+a.d[ 1]*b.d[ 6]+a.d[ 2]*b.d[10]+a.d[ 3]*b.d[14], a.d[ 0]*b.d[ 3]+a.d[ 1]*b.d[ 7]+a.d[ 2]*b.d[11]+a.d[ 3]*b.d[15],
      a.d[ 4]*b.d[ 0]+a.d[ 5]*b.d[ 4]+a.d[ 6]*b.d[ 8]+a.d[ 7]*b.d[12], a.d[ 4]*b.d[ 1]+a.d[ 5]*b.d[ 5]+a.d[ 6]*b.d[ 9]+a.d[ 7]*b.d[13], a.d[ 4]*b.d[2 ]+a.d[ 5]*b.d[ 6]+a.d[ 6]*b.d[10]+a.d[ 6]*b.d[14], a.d[ 4]*b.d[ 3]+a.d[ 5]*b.d[ 7]+a.d[ 6]*b.d[11]+a.d[ 7]*b.d[15],
      a.d[ 8]*b.d[ 0]+a.d[ 9]*b.d[ 4]+a.d[10]*b.d[ 8]+a.d[11]*b.d[12], a.d[ 8]*b.d[ 1]+a.d[ 9]*b.d[ 5]+a.d[10]*b.d[ 9]+a.d[11]*b.d[13], a.d[ 8]*b.d[2 ]+a.d[ 9]*b.d[ 6]+a.d[10]*b.d[10]+a.d[11]*b.d[14], a.d[ 8]*b.d[ 3]+a.d[ 9]*b.d[ 7]+a.d[10]*b.d[11]+a.d[11]*b.d[15],
      a.d[12]*b.d[ 0]+a.d[13]*b.d[ 4]+a.d[14]*b.d[ 8]+a.d[15]*b.d[12], a.d[12]*b.d[ 1]+a.d[13]*b.d[ 5]+a.d[14]*b.d[ 9]+a.d[15]*b.d[13], a.d[12]*b.d[2 ]+a.d[13]*b.d[ 6]+a.d[14]*b.d[10]+a.d[15]*b.d[14], a.d[12]*b.d[ 3]+a.d[13]*b.d[ 7]+a.d[14]*b.d[11]+a.d[15]*b.d[15],
  };
}

template <typename T>
Matrix4<T> operator*(Matrix4<T> a, T b)
{
  return Matrix4<T>{
    a.d[0]*b,  a.d[1]*b,  a.d[2]*b,  a.d[3]*b,
    a.d[4]*b,  a.d[5]*b,  a.d[6]*b,  a.d[7]*b,
    a.d[8]*b,  a.d[9]*b,  a.d[10]*b, a.d[11]*b,
    a.d[12]*b, a.d[13]*b, a.d[14]*b, a.d[15]*b,
  };
}

template <typename T>
inline Matrix4<T> Matrix4<T>::inverse() const
{
  Matrix4 x;
  T det;

  x.d[0]  = +d[5]*d[10]*d[15] - d[5]*d[11]*d[14] - d[9]*d[6]*d[15] + d[9]*d[7]*d[14] + d[13]*d[6]*d[11] - d[13]*d[7]*d[10];
  x.d[1]  = -d[1]*d[10]*d[15] + d[1]*d[11]*d[14] + d[9]*d[2]*d[15] - d[9]*d[3]*d[14] - d[13]*d[2]*d[11] + d[13]*d[3]*d[10];
  x.d[2]  = +d[1]*d[6]*d[15] - d[1]*d[7]*d[14] - d[5]*d[2]*d[15] + d[5]*d[3]*d[14] + d[13]*d[2]*d[7] - d[13]*d[3]*d[6];
  x.d[3]  = -d[1]*d[6]*d[11] + d[1]*d[7]*d[10] + d[5]*d[2]*d[11] - d[5]*d[3]*d[10] - d[9]*d[2]*d[7] + d[9]*d[3]*d[6];
  x.d[4]  = -d[4]*d[10]*d[15] + d[4]*d[11]*d[14] + d[8]*d[6]*d[15] - d[8]*d[7]*d[14] - d[12]*d[6]*d[11] + d[12]*d[7]*d[10];
  x.d[5]  = +d[0]*d[10]*d[15] - d[0]*d[11]*d[14] - d[8]*d[2]*d[15] + d[8]*d[3]*d[14] + d[12]*d[2]*d[11] - d[12]*d[3]*d[10];
  x.d[6]  = -d[0]*d[6]*d[15] + d[0]*d[7]*d[14] + d[4]*d[2]*d[15] - d[4]*d[3]*d[14] - d[12]*d[2]*d[7] + d[12]*d[3]*d[6];
  x.d[7]  = +d[0]*d[6]*d[11] - d[0]*d[7]*d[10] - d[4]*[2]*d[11] + d[4]*d[3]*d[10] + d[8]*d[2]*d[7] - d[8]*d[3]*d[6];
  x.d[8]  = +d[4]*d[9]*d[15] - d[4]*d[11]*d[13] - d[8]*d[5]*d[15] + d[8]*d[7]*d[13] + d[12]*d[5]*d[11] - d[12]*d[7]*d[9];
  x.d[9]  = -d[0]*d[9]*d[15] + d[0]*d[11]*d[13] + d[8]*d[1]*d[15]- d[8]*d[3]*d[13] - d[12]*d[1]*d[11] + d[12]*d[3]*d[9];
  x.d[10] = +d[0]*d[5]*d[15] - d[0]*d[7]*d[13] - d[4]*d[1]*d[15] + d[4]*d[3]*d[13] + d[12]*d[1]*d[7] - d[12]*d[3]*d[5];
  x.d[11] = -d[0]*d[5]*d[11] + d[0]*d[7]*d[9] + d[4]*d[1]*d[11] - d[4]*d[3]*d[9] - d[8]*d[1]*d[7] + d[8]*d[3]*d[5];
  x.d[12] = -d[4]*d[9]*d[14] + d[4]*d[10]*d[13] + d[8]*d[5]*d[14] - d[8]*d[6]*d[13] - d[12]*d[5]*d[10] + d[12]*d[6]*d[9];
  x.d[13] = +d[0]*d[9]*d[14] - d[0]*d[10]*d[13] - d[8]*d[1]*d[14] + d[8]*d[2]*d[13] + d[12]*d[1]*d[10] - d[12]*d[2]*d[9];
  x.d[14] = -d[0]*d[5]*d[14] + d[0]*d[6]*d[13] + d[4]*d[1]*d[14] - d[4]*d[2]*d[13] - d[12]*d[1]*d[6] + d[12]*d[2]*d[5];
  x.d[15] = +d[0]*d[5]*d[10] - d[0]*d[6]*d[9] - d[4]*d[1]*d[10] + d[4]*d[2]*d[9] + d[8]*d[1]*d[6] - d[8]*d[2]*d[5];

  det = d[0]*x.d[0] + d[1]*x.d[4] + d[2]*x.d[8] + d[3]*x.d[12];
  det = (T)(1.0 / (double)det);

  x *= det;

  return x;
}

using mat4  = Matrix4<float>;
using imat4 = Matrix4<int>;

inline mat4 operator*(mat4 a, mat4 b)
{
  mat4 c;

  intrin::mat4_mult(a, b, c);
  return c;
}

mat4 mat4::transpose() const
{
  mat4 b;

  intrin::mat4_transpose(this->d, b);
  return b;
}

// Must be non-singular!
mat4 mat4::inverse() const
{
  mat4 b;

  intrin::mat4_inverse(this->d, b);
  return b;
}

template <typename T>
Vector4<T> operator*(Matrix4<T> a, Vector4<T> b)
{
  return Vector4<T>{
    a.d[ 0]*b.x+a.d[ 1]*b.y+a.d[ 2]*b.z+a.d[ 3]*b.w,
    a.d[ 4]*b.x+a.d[ 5]*b.y+a.d[ 6]*b.z+a.d[ 7]*b.w,
    a.d[ 8]*b.x+a.d[ 9]*b.y+a.d[10]*b.z+a.d[11]*b.w,
    a.d[12]*b.x+a.d[14]*b.y+a.d[14]*b.z+a.d[15]*b.w,
  };
}

inline vec4 operator*(mat4 a, vec4 b)
{
  alignas(16) vec4 c;

  intrin::mat4_vec4_mult(a, b, c);
  return c;
}

template <typename T>
T lerp(T a, T b, float u)
{
  return a + (b-a)*u;
}

static vec4 lerp(vec4 a, vec4 b, float u)
{
  alignas(16) vec4 c;

  intrin::vec4_lerp(a, b, u, c);
  return c;
}

static vec3 lerp(vec3 a, vec3 b, float u)
{
  alignas(16) vec4 aa(a);
  alignas(16) vec4 bb(b);

  alignas(16) vec4 c;

  intrin::vec4_lerp(aa, bb, u, c);
  return c.xyz();
}

template <typename T>
T clamp(T x, T minimum, T maximum)
{
  if(x <= minimum) return minimum;
  
  return std::min(x, maximum);
}

static vec2 clamp(vec2 x, vec2 minimum, vec2 maximum)
{
  return {
    clamp(x.x, minimum.x, maximum.x),
    clamp(x.y, minimum.y, maximum.y)
  };
}

static vec3 clamp(vec3 x, vec3 minimum, vec3 maximum)
{
  return {
    clamp(x.x, minimum.x, maximum.x),
    clamp(x.y, minimum.y, maximum.y),
    clamp(x.z, minimum.z, maximum.z)
  };
}

static vec4 clamp(vec4 x, vec4 minimum, vec4 maximum)
{
  return {
    clamp(x.x, minimum.x, maximum.x),
    clamp(x.y, minimum.y, maximum.y),
    clamp(x.z, minimum.z, maximum.z),
    clamp(x.w, minimum.w, maximum.w)
  };
}

template <typename Limit, typename T>
Limit saturate(T x)
{
  static_assert(std::numeric_limits<T>::digits > std::numeric_limits<Limit>::digits,
                "saturating smaller type with larger!");

  using Limits = std::numeric_limits<Limit>;
  return (Limit)clamp(x, (T)Limits::min(), (T)Limits::max());
}

template <typename T>
float smoothstep(T min, T max, float u)
{
  u = clamp((u - min) / (max - min), 0.0f, 1.0f);
  return u*u*(3 - 2*u);
}

namespace xform {

static mat4 identity()
{
  return mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 translate(float x, float y, float z)
{
  return mat4{
      1.0f, 0.0f, 0.0f, x,
      0.0f, 1.0f, 0.0f, y,
      0.0f, 0.0f, 1.0f, z,
      0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 translate(vec3 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

static mat4 translate(vec4 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

static mat4 scale(float x, float y, float z)
{
  return mat4{
      x,    0.0f, 0.0f, 0.0f,
      0.0f, y,    0.0f, 0.0f,
      0.0f, 0.0f, z,    0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 scale(float s)
{
  return scale(s, s, s);
}

static mat4 rotx(float a)
{
  return mat4{
      1.0f, 0.0f,    0.0f,     0.0f,
      0.0f, cosf(a), -sinf(a), 0.0f,  
      0.0f, sinf(a), cosf(a),  0.0f,
      0.0f, 0.0f,    0.0f,     1.0f,
  };
}

static mat4 roty(float a)
{
  return mat4{
       cosf(a), 0.0f, sinf(a), 0.0f,
      0.0f,     1.0f, 0.0f,    0.0f,
      -sinf(a), 0.0f, cosf(a), 0.0f,
      0.0f,     0.0f, 0.0f,    1.0f,
  };
}

static mat4 rotz(float a)
{
  return mat4{
      cosf(a), -sinf(a), 0.0f, 0.0f,
      sinf(a), cosf(a),  0.0f, 0.0f,
      0.0f,    0.0f,     1.0f, 0.0f,
      0.0f,    0.0f,     0.0f, 1.0f,
  };
}

static mat4 ortho(float t, float l, float b, float r, float n, float f)
{
  return mat4{
    2.0f/(r-l), 0.0f,       0.0f,        -(r+l)/(r-l),
    0.0f,       2.0f/(t-b), 0.0f,        -(t+b)/(t-b),
    0.0f,       0.0f,       -2.0f/(f-n), -(f+n)/(f-n),
    0.0f,       0.0f,       0.0f,        1.0f,
  };
}

static mat4 frutsum(float t, float l, float b, float r, float n, float f)
{
  auto n2 = n*2.0f;
  return mat4{
    n2/(r-l), 0.0f,     (r+l)/(r-l),  0.0f,
    0.0f,     n2/(t-b), (t+b)/(t-b),  0.0f,
    0.0f,     0.0f,     -(f+n)/(f-n), -f*n2/(f-n),
    0.0f,     0.0f,     -1.0f,        0.0f,

  };
}

static mat4 perspective(float fovy, float aspect, float n, float f)
{
  float w, h;

  h = tanf(fovy / (360.0f*PIf)) * n;
  w = h * aspect;

  return frutsum(h, -w, -h, w, n, f);
}

static mat4 look_at(vec3 eye, vec3 target, vec3 up)
{
  vec3 forward = (eye-target).normalize();
  vec3 left = up.cross(forward).normalize();

  up = forward.cross(left);

  float x = -left.x*eye.x - left.y*eye.y - left.z*eye.z,
    y = -up.x*eye.x - up.y*eye.y - up.z*eye.z,
    z = -forward.x*eye.x - forward.y*eye.y - forward.z*eye.z;

  return mat4{
    left.x,    left.y,    left.z,    x,
    up.x,      up.y,      up.z,      y,
    forward.x, forward.y, forward.z, z,
    0.0f,      0.0f,      0.0f,      1.0f,
  };

}

static vec2 project(vec4 v, mat4 modelviewprojection, ivec2 screen)
{
  v = modelviewprojection * v;
  v *= 1.0f/v.w;

  vec2 screenf = { (float)screen.x, (float)screen.y };

  return {
    (v.x+1)/2 * screenf.x,
    screenf.y - ((v.y+1)/2 * screenf.y)
  };
}

}

#pragma pack(pop)