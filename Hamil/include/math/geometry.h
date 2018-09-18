#pragma once

#include <math/intrin.h>

#include <cmath>
#include <array>
#include <algorithm>
#include <limits>
#include <type_traits>

constexpr double PI = 3.1415926535897932384626433832795;
constexpr float PIf = (float)PI;

#pragma pack(push, 1)

template <typename T>
struct Vector2 {
  constexpr Vector2() :
    x(0), y(0)
  { }
  constexpr Vector2(T x_, T y_) :
    x(x_), y(y_)
  { }
  Vector2(const T *v) :
    x(v[0]), y(v[1])
  { }

  union {
    struct { T x, y; };
    struct { T s, t; };
  };

  T length2() const { return dot(*this); }
  T length() const { return (T)sqrt(length2()); }
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
  constexpr Vector3() :
    x(0), y(0), z(0)
  { }
  constexpr Vector3(T x_, T y_, T z_) :
    x(x_), y(y_), z(z_)
  { }
  constexpr Vector3(const Vector2<T>& v, T z_) :
    x(v.x), y(v.y), z(z_)
  { }
  constexpr Vector3(T v) :
    x(v), y(v), z(v)
  { }
  Vector3(const T *v) :
    x(v[0]), y(v[1]), z(v[2])
  { }

  union {
    struct { T x, y, z; };
    struct { T r, g, b; };
    struct { T s, t, p; };
  };

  // Returns a unit vector which points from 'a' to 'b'
  static Vector3 direction(const Vector3& a, const Vector3& b)
  {
    return (b-a).normalize();
  }

  Vector2<T> xy() const { return Vector2<T>{ x, y }; }

  T length2() const { return dot(*this); }
  T length() const { return (T)sqrt(length2()); }
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

  bool isZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }

  bool zeroLength() const
  {
    auto l = length2();
    return  l < (1e-6f * 1e-6f);
  }

  Vector3 operator-() const
  {
    return { -x, -y, -z };
  }

  static Vector3 zero()    { return { (T)0, (T)0, (T)0 }; }
  static Vector3 up()      { return { (T)0, (T)1, (T)0 }; }
  static Vector3 left()    { return { (T)-1, (T)0, (T)0 }; }
  static Vector3 down()    { return { (T)0, (T)-1, (T)0 }; }
  static Vector3 right()   { return { (T)1, (T)0, (T)0 }; }
  static Vector3 forward() { return { (T)0, (T)0, (T)1 }; }
  static Vector3 back()    { return { (T)0, (T)0, (T)-1 }; }

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

// Extended to 4 floats and aligned siutably for SSE
struct alignas(16) intrin_vec3 {
  float d[4];

  intrin_vec3(const vec3& v, float w = 0.0f) :
    d{ v.x, v.y, v.z, w }
  { }
  intrin_vec3()
  { }

  vec3 toVec3() const { return vec3(d); }

  operator float *() { return d; }
};

inline float vec3::dot(const vec3& v) const
{
  auto a = intrin_vec3(*this);
  auto b = intrin_vec3(v);
  float c[1];

  intrin::vec_dot(a, b, c);
  return *c;
}

inline vec3 vec3::normalize() const
{
  auto a = intrin_vec3(*this);
  auto b = intrin_vec3();
  
  intrin::vec_normalize(a, b);
  return b.toVec3();
}

inline vec3 vec3::cross(const vec3& v) const
{
  auto a = intrin_vec3(*this);
  auto b = intrin_vec3(v);
  auto c = intrin_vec3();

  intrin::vec3_cross(a, b, c);
  return c.toVec3();
}

inline vec3 vec3::recip() const
{
  auto a = intrin_vec3(*this);
  auto b = intrin_vec3();

  intrin::vec_recip(a, b);
  return b.toVec3();
}

template <typename T>
struct /* alignas(16) for intrin */ Vector4 {
  constexpr Vector4() :
    x(0), y(0), z(0), w(1)
  { }
  constexpr Vector4(T x_, T y_, T z_, T w_) :
    x(x_), y(y_), z(z_), w(w_)
  { }
  constexpr Vector4(Vector2<T> xy, T z_, T w_) :
    x(xy.x), y(xy.y), z(z_), w(w_)
  { }
  constexpr Vector4(Vector2<T> xy, T z_) :
    x(xy.x), y(xy.y), z(z_), w(1)
  { }
  constexpr Vector4(Vector2<T> xy) :
    x(xy.x), y(xy.y), z(0), w(1)
  { }
  constexpr Vector4(Vector3<T> xyz) :
    x(xyz.x), y(xyz.y), z(xyz.z), w(1.0f)
  { }
  constexpr Vector4(Vector3<T> xyz, T w_) :
    x(xyz.x), y(xyz.y), z(xyz.z), w(w_)
  { }
  Vector4(const T *v) :
    x(v[0]), y(v[1]), z(v[2]), w(v[3])
  { }

  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    struct { T s, t, p, q; };
  };

  // Returns a unit vector which points from 'a' to 'b'
  static Vector4 direction(const Vector4& a, const Vector4& b)
  {
    return (b-a).normalize();
  }

  Vector3<T> xyz() const { return Vector3<T>{ x, y, z }; }

  T length2() const { return dot(*this); }
  T length() const { return (T)sqrt(length2()); }
  T dot(const Vector4& b) const { return x*b.x + y*b.y + z*b.z + w*b.w; }

  Vector4 normalize() const
  {
    T l = (T)1 / length();
    return (*this) * l;
  }

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

inline float vec4::dot(const vec4& b) const
{
  alignas(16) float d[1];

  intrin::vec_dot(*this, b, d);
  return *d;
}

inline vec4 vec4::normalize() const
{
  alignas(16) vec4 b;

  intrin::vec_normalize(*this, b);
  return b;
}

inline vec4 vec4::recip() const
{
  alignas(16) vec4 b;

  intrin::vec_recip(*this, b);
  return b;
}

template <typename T>
struct Matrix3;

template <typename T>
struct Matrix4;

template <typename T>
struct Matrix2 {
  T d[2*2];

  T operator()(unsigned col, unsigned row) const { return d[col*2 + row]; }

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

  T operator()(unsigned col, unsigned row) const { return d[col*3 + row]; }

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
struct alignas(16) Matrix4 {
  T d[4*4];

  T operator()(unsigned col, unsigned row) const { return d[col*4 + row]; }

  static Matrix4 from_mat3(const Matrix3<T>& m)
  {
    return {
      m[0], m[1], m[2], 0.0f,
      m[3], m[4], m[5], 0.0f,
      m[6], m[7], m[8], 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  Matrix4& operator *=(const Matrix4& b) { *this = *this * b; return *this; }

  Matrix4 transpose() const
  {
    return Matrix4{
      d[0], d[4], d[8],  d[12],
      d[1], d[5], d[9],  d[13],
      d[2], d[6], d[10], d[14],
      d[3], d[7], d[11], d[15],
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

  Vector3<T> translation() const
  {
    return { d[3], d[7], d[11] };
  }

  operator float *() { return d; }
  operator const float *() const { return d; }
};

template <typename T>
Matrix4<T> operator+(const Matrix4<T>& a, const Matrix4<T>& b)
{
  return {
    a[0]+b[0],   a[1]+b[1],   a[2]+b[2],   a[3]+b[3],
    a[4]+b[4],   a[5]+b[5],   a[6]+b[6],   a[7]+b[7],
    a[8]+b[8],   a[9]+b[9],   a[10]+b[10], a[11]+b[11],
    a[12]+b[12], a[13]+b[13], a[14]+b[14], a[15]+b[15],
  };
}

template <typename T>
Matrix4<T> operator*(const Matrix4<T>& a, const Matrix4<T>& b)
{
  return Matrix4<T>{
      a.d[ 0]*b.d[ 0]+a.d[ 1]*b.d[ 4]+a.d[ 2]*b.d[ 8]+a.d[ 3]*b.d[12], a.d[ 0]*b.d[ 1]+a.d[ 1]*b.d[ 5]+a.d[ 2]*b.d[ 9]+a.d[ 3]*b.d[13], a.d[ 0]*b.d[2 ]+a.d[ 1]*b.d[ 6]+a.d[ 2]*b.d[10]+a.d[ 3]*b.d[14], a.d[ 0]*b.d[ 3]+a.d[ 1]*b.d[ 7]+a.d[ 2]*b.d[11]+a.d[ 3]*b.d[15],
      a.d[ 4]*b.d[ 0]+a.d[ 5]*b.d[ 4]+a.d[ 6]*b.d[ 8]+a.d[ 7]*b.d[12], a.d[ 4]*b.d[ 1]+a.d[ 5]*b.d[ 5]+a.d[ 6]*b.d[ 9]+a.d[ 7]*b.d[13], a.d[ 4]*b.d[2 ]+a.d[ 5]*b.d[ 6]+a.d[ 6]*b.d[10]+a.d[ 6]*b.d[14], a.d[ 4]*b.d[ 3]+a.d[ 5]*b.d[ 7]+a.d[ 6]*b.d[11]+a.d[ 7]*b.d[15],
      a.d[ 8]*b.d[ 0]+a.d[ 9]*b.d[ 4]+a.d[10]*b.d[ 8]+a.d[11]*b.d[12], a.d[ 8]*b.d[ 1]+a.d[ 9]*b.d[ 5]+a.d[10]*b.d[ 9]+a.d[11]*b.d[13], a.d[ 8]*b.d[2 ]+a.d[ 9]*b.d[ 6]+a.d[10]*b.d[10]+a.d[11]*b.d[14], a.d[ 8]*b.d[ 3]+a.d[ 9]*b.d[ 7]+a.d[10]*b.d[11]+a.d[11]*b.d[15],
      a.d[12]*b.d[ 0]+a.d[13]*b.d[ 4]+a.d[14]*b.d[ 8]+a.d[15]*b.d[12], a.d[12]*b.d[ 1]+a.d[13]*b.d[ 5]+a.d[14]*b.d[ 9]+a.d[15]*b.d[13], a.d[12]*b.d[2 ]+a.d[13]*b.d[ 6]+a.d[14]*b.d[10]+a.d[15]*b.d[14], a.d[12]*b.d[ 3]+a.d[13]*b.d[ 7]+a.d[14]*b.d[11]+a.d[15]*b.d[15],
  };
}

template <typename T>
Matrix4<T> operator*(const Matrix4<T>& a, T b)
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

inline mat4 operator*(const mat4& a, const mat4& b)
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
Vector4<T> operator*(const Matrix4<T>& a, const Vector4<T>& b)
{
  return Vector4<T>{
    a.d[ 0]*b.x+a.d[ 1]*b.y+a.d[ 2]*b.z+a.d[ 3]*b.w,
    a.d[ 4]*b.x+a.d[ 5]*b.y+a.d[ 6]*b.z+a.d[ 7]*b.w,
    a.d[ 8]*b.x+a.d[ 9]*b.y+a.d[10]*b.z+a.d[11]*b.w,
    a.d[12]*b.x+a.d[13]*b.y+a.d[14]*b.z+a.d[15]*b.w,
  };
}

inline vec4 operator*(const mat4& a, const vec4& b)
{
  alignas(16) vec4 c;

  intrin::mat4_vec4_mult(a, b, c);
  return c;
}

#pragma pack(pop)