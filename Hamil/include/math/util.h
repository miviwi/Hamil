#pragma once

#include <math/geometry.h>
#include <math/quaternion.h>

#include <cassert>
#include <string>
#include <vector>
#include <utility>

// Returns 'v' rounded to the nearest power-of-2
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

// Aligns 'x' to the power-of-2 passed in 'alignment'
//   - 'alignment' MUST be a power-of-2!
static unsigned pow2_align(unsigned x, unsigned alignment)
{
  assert(pow2_round(alignment) == alignment && "pow2_align() alignment is not a power of 2!");

  return (x + (alignment-1)) & ~(alignment-1);
}

// Loads the big-endian u32 pointed to by 'ptr'
//   and returns it
u32 loadbe_u32(const void *ptr);
// Stores 'v' as a big-endian u32 in memory
//   pointed to by 'ptr'
void storebe_u32(u32 v, void *ptr);

// Does NOT use F16C instructions
u16 to_f16(float f);
// Does NOT use F16C instructions
float from_f16(u16 h);

hvec2 to_f16(const vec2& v);
vec2 from_f16(const hvec2& v);

hvec3 to_f16(const vec3& v);
vec3 from_f16(const hvec3& v);

hvec4 to_f16(const vec4& v);
vec4 from_f16(const hvec4& v);

template <typename T>
T lerp(T a, T b, float u)
{
  return a + (b-a)*u;
}

static double lerp(double a, double b, double u)
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
  alignas(16) intrin_vec3 aa(a);
  alignas(16) intrin_vec3 bb(b);

  alignas(16) intrin_vec3 c;

  intrin::vec4_lerp(aa, bb, u, c);
  return c.toVec3();
}

template <typename T, typename U>
T clamp(U x, T minimum, T maximum)
{
  if(x <= (U)minimum) return minimum;

  return (T)std::min<U>(x, maximum);
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

// Returns:
//    numeric_limits<Limit>::min() when x < min(),
//    numeric_limits<Limit>::max() when x > max(),
//    and x otherwise
template <typename Limit, typename T>
Limit saturate(T x)
{
  using Limits = std::numeric_limits<Limit>;
  return clamp<Limit>(x, Limits::min(), Limits::max());
}

// When x<edge returns 0 and 1 otherwise
template <typename T>
T step(T edge, T x)
{
  return x < edge ? (T)0 : (T)1;
}

// Interpolates between min and max using a Hermite curve
//   with u between [0;1] being analogous to lerp's 'alpha'
template <typename T>
T smoothstep(T min, T max, float u)
{
  u = clamp((u - min) / (max - min), 0.0f, 1.0f);
  return u*u*(3 - 2*u);
}

// r == 1   =>  3x3 kernel
// r == 2   =>  5x5 kernel
// r == 3   =>  7x7 kernel
//  etc...
// Returns a vector where the first r*2 + 1 elements
//   are the kernel itself and the last element is
//   the normalization factor
std::vector<float> gaussian_kernel(int r);

namespace math {

std::string to_str(const vec2& v);
std::string to_str(const ivec2& v);
std::string to_str(const vec3& v);
std::string to_str(const vec4& v);
std::string to_str(const mat3& m);
std::string to_str(const mat4& m);
std::string to_str(const quat& q);

}