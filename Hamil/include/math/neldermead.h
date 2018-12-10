#pragma once

#include <common.h>

#include <cmath>
#include <array>

template <int Dim, typename Func>
float nedler_mead(float *pmin, const float *start, float delta, float tolerance,
  int max_iters, Func objective_fn)
{
  static constexpr float
    reflect  = 1.0f,
    expand   = 2.0f,
    contract = 0.5f,
    shrink   = 0.5f;

  using Point = std::array<float, Dim>;
  enum {
    NumPoints = Dim + 1,
  };

  Point s[NumPoints];
  float f[NumPoints];

  auto point_each = [](Point& dst, auto fn /* float (*)(int) */) {
    for(int i = 0; i < Dim; i++) dst[i] = fn(i);
  };

  auto mov = [](float *dst, const float *src) {
    for(size_t i = 0; i < Dim; i++) dst[i] = src[i];
  };

  auto set = [](float *dst, float v) {
    for(size_t i = 0; i < Dim; i++) dst[i] = v;
  };

  auto add = [](float *dst, const float *src) {
    for(size_t i = 0; i < Dim; i++) dst[i] += src[i];
  };

  mov(s[0].data(), start);
  for(int i = 1; i < NumPoints; i++) {
    mov(s[i].data(), start);
    s[i][i - 1] += delta;
  }

  for(int i = 0; i < NumPoints; i++) f[i] = objective_fn(s[i].data());

  int lo = 0, hi = ~0, nh = 0;
  for(int j = 0; j < max_iters; j++) {
    lo = hi = nh = 0;
    for(int i = 1; i < NumPoints; i++) {
      if(f[i] < f[lo]) lo = i;

      if(f[i] > f[hi]) {
        nh = hi;
        hi = i;
      } else if(f[i] > f[nh]) {
        nh = i;
      }
    }

    // Stop if we've reached the required tolerance
    float a = fabsf(f[lo]), b = fabsf(f[hi]);
    if(2.0f*fabsf(a - b) < (a+b)*tolerance) break;

    Point o;
    set(o.data(), 0.0f);
    for(int i = 0; i < NumPoints; i++) {
      if(i == hi) continue;

      add(o.data(), s[i].data());
    }

    point_each(o, [&](int i) { return o[i] / (float)Dim; });

    // relfection
    Point r;
    point_each(r, [&](int i) { return o[i] + reflect*(o[i] - s[hi][i]); });

    float fr = objective_fn(r.data());
    if(fr < f[nh]) {
      if(fr < f[lo]) {
        // expansion
        Point e;
        point_each(e, [&](int i) { return o[i] + expand*(o[i] - s[hi][i]); });

        float fe = objective_fn(e.data());
        if(fe < fr) {
          mov(s[hi].data(), e.data());
          f[hi] = fe;
          continue;
        }
      }

      mov(s[hi].data(), r.data());
      f[hi] = fr;
      continue;
    }

    // contraction
    Point c;
    point_each(c, [&](int i) { return o[i] - contract*(o[i] - s[hi][i]); });

    float fc = objective_fn(c.data());
    if(fc < f[hi]) {
      mov(s[hi].data(), c.data());
      f[hi] = fc;
      continue;
    }

    // reduction
    for(int k = 0; k < NumPoints; k++) {
      if(k == 0) continue;

      point_each(s[k], [&](int i) {
        return s[lo][i] + shrink*(s[k][i] - s[lo][i]);
      });

      f[k] = objective_fn(s[k].data());
    }
  }

  mov(pmin, s[lo].data());
  return f[lo];
}