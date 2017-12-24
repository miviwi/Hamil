#pragma once

#include <cmath>

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

#pragma pack(push, 1)

template <typename T>
struct Vector2 {
  union {
    struct { T x, y; };
    struct { T u, v; };
  };

  T length() { return (T)sqrt((x*x) + (y*y)); }
  T dot(const Vector2& b) { return (a.x*b.x) + (a.y*b.y); }

  operator float *() { return (float *)this; }
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
Vector2<T> operator/(Vector2<T> a, Vector2<T> b)
{
  return Vector2<T>{ a.x/b.x, a.y/b.y };
}

template <typename T>
Vector2<T> operator*(Vector2<T> a, T u)
{
  return Vector2<T>{ a.x*u, a.y*u };
}

using vec2 = Vector2<float>;
using ivec2 = Vector2<int>;

template <typename T>
struct Vector3 {
  union {
    struct { T x, y, z; };
    struct { T r, g, b; };
    struct { T u, v; };
  };

  T length() { return (T)sqrt((x*x) + (y*y) + (z*z)); }
  T dot(const Vector3& b) { return (a.x*b.x) + (a.y*b.y) + (a.z*b.z); }

  operator float *() { return (float *)this; }
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
Vector3<T> operator/(Vector3<T> a, Vector3<T> b)
{
  return Vector3<T>{ a.x/b.x, a.y/b.y, a.z/b.z };
}

template <typename T>
Vector3<T> operator*(Vector3<T> a, T u)
{
  return Vector3<T>{ a.x*u, a.y*u, a.z*u };
}

using vec3 = Vector3<float>;
using ivec3 = Vector3<int>;

template <typename T>
struct alignas(16) Vector4 {
  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    struct { T u, v; };
  };

  T length() { return (T)sqrt((x*x) + (y*y) + (z*z) + (w*w)); }
  T dot(const Vector4& b) { return (a.x*b.x) + (a.y*b.y) + (a.z*b.z) + (a.w*b.w); }

  operator float *() { return (float *)this; }
};

template <typename T>
Vector4<T> operator+(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w };
}

template <typename T>
Vector4<T> operator-(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w };
}

template <typename T>
Vector4<T> operator*(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w };
}

template <typename T>
Vector4<T> operator/(Vector4<T> a, Vector4<T> b)
{
  return Vector4<T>{ a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w };
}

template <typename T>
Vector4<T> operator*(Vector4<T> a, T u)
{
  return Vector4<T>{ a.x*u, a.y*u, a.z*u, a.w*u };
}

using vec4 = Vector4<float>;
using ivec4 = Vector4<int>;

template <typename T>
struct Matrix2 {
  T d[2*2];

  Matrix2& operator *=(Matrix2& b) { *this = *this * b; return *this; }

  operator float *() { return d; }
};

template <typename T>
Matrix2<T> operator*(Matrix2<T> a, Matrix2<T> b)
{
  return Matrix2<T>{
      a.d[0]*b.d[0]+a.d[1]*b.d[2], a.d[0]*b.d[1]+a.d[1]*b.d[3],
      a.d[2]*b.d[0]+a.d[3]*b.d[2], a.d[2]*b.d[1]+a.d[3]*b.d[3],
  };
}

using mat2 = Matrix2<float>;
using imat2 = Matrix2<int>;

template <typename T>
struct Matrix3 {
  T d[3*3];

  Matrix3& operator *=(Matrix3& b) { *this = *this * b; return *this; }

  operator float *() { return d; }
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

using mat3 = Matrix3<float>;
using imat3 = Matrix3<int>;

template <typename T>
struct Matrix4 {
  alignas(16) T d[4*4];

  Matrix4& operator *=(Matrix4& b) { *this = *this * b; return *this; }

  operator float *() { return d; }
};

namespace intrin {
void mat4_mult(const float *a, const float *b, float *out);
}

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

using mat4 = Matrix4<float>;
using imat4 = Matrix4<int>;

static mat4 operator*(mat4 a, mat4 b)
{
  mat4 c;

  intrin::mat4_mult(a, b, c);
  return c;
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

namespace intrin {
void mat4_vec4_mult(const float *a, const float *b, float *out);
}

static vec4 operator*(mat4 a, vec4 b)
{
  vec4 c;

  intrin::mat4_vec4_mult(a, b, c);
  return c;
}

template <typename T>
T lerp(T a, T b, float u)
{
  return a + (b-a)*u;
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

static mat4 scale(float x, float y, float z)
{
  return mat4{
      x,    0.0f, 0.0f, 0.0f,
      0.0f, y,    0.0f, 0.0f,
      0.0f, 0.0f, z,    0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
  };
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
  return mat4{
    2.0f*n/(r-l), 0.0f,         (r+l)/(r-l),  0.0f,
    0.0f,         2.0f*n/(t-b), (t+b)/(t-b),  0.0f,
    0.0f,         0.0f,         -(f+n)/(f-n), -2.0f*f*n/(f-n),
    0.0f,         0.0f,         -1.0f,        0.0f,

  };
}

static mat4 perspective(float fovy, float aspect, float n, float f)
{
  float w, h;

  h = tan(fovy / (360.0*PI)) * n;
  w = h * aspect;

  return frutsum(h, -w, -h, w, n, f);
}

}

#pragma pack(pop)