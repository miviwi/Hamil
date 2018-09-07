#pragma once

#include <math/geometry.h>

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