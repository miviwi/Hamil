#pragma once

namespace glang {

template <typename T, unsigned N>
inline T sign_extend(T x)
{
  struct { T x : N; } s;
  return s.x = x;
}

}