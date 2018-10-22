#pragma once

#include <xmmintrin.h>
#include <pmmintrin.h>

#if !defined(NO_AVX)
#  include <smmintrin.h>
#  include <immintrin.h>
#endif

#include <cassert>

namespace intrin {

#define ps_epi32(r) _mm_castps_si128(r)
#define epi32_ps(r) _mm_castsi128_ps(r)

#if defined(NO_AVX)
#  define pshufd(r, i) epi32_ps(_mm_shuffle_epi32(ps_epi32(r), i))
#else
#  define pshufd(r, i) _mm_permute_ps(r, i)
#endif

// The selectors are REVERSED compared to the Intel ordering i.e. the LOWEST component goes FIRST
#define shuffle_ps(r, _0, _1, _2, _3)     pshufd(r, _MM_SHUFFLE(_3, _2, _1, _0))
#define shuffle2_ps(p, r, _0, _1, _2, _3) _mm_shuffle_ps(p, r, _MM_SHUFFLE(_3, _2, _1, _0))

#define broadcast_ps(r, i) pshufd(r, _MM_SHUFFLE(i, i, i, i))

// The arguments are REVERSED like shuffle_ps
#define neg_ps(r, _0, _1, _2, _3) \
  _mm_xor_ps(r, _mm_set_ps(_3 ? -0.0f : 0.0f, _2 ? -0.0f : 0.0f, _1 ? -0.0f : 0.0f, _0 ? -0.0f : 0.0f))

// Clears the W component of a given vector
#define mask_xyz_ps(r) \
  _mm_and_ps(r, epi32_ps(_mm_set_epi32(0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)))

// Yields the a.dot(b) in all the components
#if defined(NO_AVX)
#define dot_ps(a, b)                                   \
  broadcast_ps(                                        \
    _mm_hadd_ps(                                       \
      _mm_hadd_ps(_mm_mul_ps(a, b), _mm_setzero_ps()), \
      _mm_setzero_ps()                                 \
    ), 0                                               \
  )
#else
#define dot_ps(a, b) _mm_dp_ps(a, b, 0xFF)
#endif

// Yields a.dot(b) of the XYZ components in all (i.e. in XYZ) of them
//   and 0.0f in W
#if defined(NO_AVX)
#define dot_xyz_ps(a, b)                               \
  shuffle_ps(                                          \
    _mm_hadd_ps(                                       \
      _mm_hadd_ps(mask_xyz_ps(_mm_mul_ps(a, b)),       \
                  _mm_setzero_ps()),                   \
      _mm_setzero_ps()                                 \
    ), 0, 0, 0, 3                                      \
  )
#else
#define dot_xyz_ps(a, b) _mm_dp_ps(a, b, 0x77)
#endif

#define cross_ps(a, b)                                                \
  _mm_sub_ps(                                                         \
    _mm_mul_ps(shuffle_ps(a, 1, 2, 0, 3), shuffle_ps(b, 2, 0, 1, 3)), \
    _mm_mul_ps(shuffle_ps(a, 2, 0, 1, 3), shuffle_ps(b, 1, 2, 0, 3))  \
  )

// Swizzle component helpers
#define X 0
#define Y 1
#define Z 2
#define W 3

void mat4_mult(const float *a, const float *b, float *out)
{
  __m128 x[] ={ _mm_load_ps(b+0), _mm_load_ps(b+4), _mm_load_ps(b+8), _mm_load_ps(b+12) };

  __m128 y0, y1, y2, y3;
  __m128 z0, z1, z2, z3;

  z0 = z1 = z2 = z3 = _mm_setzero_ps();
  for(int i = 0; i < 4; i++) {
    y0 = _mm_mul_ps(_mm_load_ps1(a+i), x[i]);
    y1 = _mm_mul_ps(_mm_load_ps1(a+4+i), x[i]);
    y2 = _mm_mul_ps(_mm_load_ps1(a+8+i), x[i]);
    y3 = _mm_mul_ps(_mm_load_ps1(a+12+i), x[i]);

    z0 = _mm_add_ps(y0, z0);
    z1 = _mm_add_ps(y1, z1);
    z2 = _mm_add_ps(y2, z2);
    z3 = _mm_add_ps(y3, z3);
  }

  _mm_store_ps(out+0, z0);
  _mm_store_ps(out+4, z1);
  _mm_store_ps(out+8, z2);
  _mm_store_ps(out+12, z3);
}

void mat4_transpose(const float *a, float *out)
{
  __m128 x[] ={ _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };

  _MM_TRANSPOSE4_PS(x[0], x[1], x[2], x[3]);

  _mm_store_ps(out+0, x[0]);
  _mm_store_ps(out+4, x[1]);
  _mm_store_ps(out+8, x[2]);
  _mm_store_ps(out+12, x[3]);
}

// Based on code provided by Intel in
// "Streaming SIMD Extensions - Inverse of 4x4 Matrix"
// (ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf)
void mat4_inverse(const float *a, float *out)
{
  __m128 x[4];
  __m128 minor[4];
  __m128 det, tmp;

  det = tmp = _mm_setzero_ps();

  tmp = _mm_loadh_pi(_mm_loadl_pi(tmp, (__m64 *)(a+0)), (__m64 *)(a+4));
  x[1] = _mm_loadh_pi(_mm_loadl_pi(x[1], (__m64 *)(a+8)), (__m64 *)(a+12));
  x[0] = _mm_shuffle_ps(tmp, x[1], 0x88);
  x[1] = _mm_shuffle_ps(x[1], tmp, 0xDD);
  tmp = _mm_loadh_pi(_mm_loadl_pi(tmp, (__m64 *)(a+2)), (__m64 *)(a+6));
  x[3] = _mm_loadh_pi(_mm_loadl_pi(x[3], (__m64 *)(a+10)), (__m64 *)(a+14));
  x[2] = _mm_shuffle_ps(tmp, x[3], 0x88);
  x[3] = _mm_shuffle_ps(x[3], tmp, 0xDD);

  tmp = _mm_mul_ps(x[2], x[3]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  minor[0] = _mm_mul_ps(x[1], tmp);
  minor[1] = _mm_mul_ps(x[0], tmp);
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[0] = _mm_sub_ps(_mm_mul_ps(x[1], tmp), minor[0]);
  minor[1] = _mm_sub_ps(_mm_mul_ps(x[0], tmp), minor[1]);
  minor[1] = _mm_shuffle_ps(minor[1], minor[1], 0x4E);
  tmp = _mm_mul_ps(x[1], x[2]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  minor[0] = _mm_add_ps(_mm_mul_ps(x[3], tmp), minor[0]);
  minor[3] = _mm_mul_ps(x[0], tmp);
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[0] = _mm_sub_ps(minor[0], _mm_mul_ps(x[3], tmp));
  minor[3] = _mm_sub_ps(_mm_mul_ps(x[0], tmp), minor[3]);
  minor[3] = _mm_shuffle_ps(minor[3], minor[3], 0x4E);
  tmp = _mm_mul_ps(_mm_shuffle_ps(x[1], x[1], 0x4E), x[3]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  x[2] = _mm_shuffle_ps(x[2], x[2], 0x4E);

  minor[0] = _mm_add_ps(_mm_mul_ps(x[2], tmp), minor[0]);
  minor[2] = _mm_mul_ps(x[0], tmp);
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[0] = _mm_sub_ps(minor[0], _mm_mul_ps(x[2], tmp));
  minor[2] = _mm_sub_ps(_mm_mul_ps(x[0], tmp), minor[2]);
  minor[2] = _mm_shuffle_ps(minor[2], minor[2], 0x4E);
  tmp = _mm_mul_ps(x[0], x[1]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  minor[2] = _mm_add_ps(_mm_mul_ps(x[3], tmp), minor[2]);
  minor[3] = _mm_sub_ps(_mm_mul_ps(x[2], tmp), minor[3]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[2] = _mm_sub_ps(_mm_mul_ps(x[3], tmp), minor[2]);
  minor[3] = _mm_sub_ps(minor[3], _mm_mul_ps(x[2], tmp));
  tmp = _mm_mul_ps(x[0], x[3]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  minor[1] = _mm_sub_ps(minor[1], _mm_mul_ps(x[2], tmp));
  minor[2] = _mm_add_ps(_mm_mul_ps(x[1], tmp), minor[2]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[1] = _mm_add_ps(_mm_mul_ps(x[2], tmp), minor[1]);
  minor[2] = _mm_sub_ps(minor[2], _mm_mul_ps(x[1], tmp));
  tmp = _mm_mul_ps(x[0], x[2]);
  tmp = _mm_shuffle_ps(tmp, tmp, 0xB1);

  minor[1] = _mm_add_ps(_mm_mul_ps(x[3], tmp), minor[1]);
  minor[3] = _mm_sub_ps(minor[3], _mm_mul_ps(x[1], tmp));
  tmp = _mm_shuffle_ps(tmp, tmp, 0x4E);
  minor[1] = _mm_sub_ps(minor[1], _mm_mul_ps(x[3], tmp));
  minor[3] = _mm_add_ps(_mm_mul_ps(x[1], tmp), minor[3]);

  det = _mm_mul_ps(x[0], minor[0]);
  det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
  det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);

  tmp = _mm_rcp_ss(det);

  det = _mm_sub_ss(_mm_add_ss(tmp, tmp), _mm_mul_ss(det, _mm_mul_ss(tmp, tmp)));
  det = _mm_shuffle_ps(det, det, 0x00);

  _mm_store_ps(out+0, _mm_mul_ps(det, minor[0]));
  _mm_store_ps(out+4, _mm_mul_ps(det, minor[1]));
  _mm_store_ps(out+8, _mm_mul_ps(det, minor[2]));
  _mm_store_ps(out+12, _mm_mul_ps(det, minor[3]));
}

void mat4_vec4_mult(const float *a, const float *b, float *out)
{
  __m128 x[] ={ _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };
  _MM_TRANSPOSE4_PS(x[0], x[1], x[2], x[3]);

  __m128 y;
  __m128 z = _mm_setzero_ps();

  y = _mm_mul_ps(_mm_load_ps1(b+0), x[0]);
  z = _mm_add_ps(y, z);

  y = _mm_mul_ps(_mm_load_ps1(b+1), x[1]);
  z = _mm_add_ps(y, z);

  y = _mm_mul_ps(_mm_load_ps1(b+2), x[2]);
  z = _mm_add_ps(y, z);

  y = _mm_mul_ps(_mm_load_ps1(b+3), x[3]);
  z = _mm_add_ps(y, z);

  _mm_store_ps(out, z);
}

void mat4_scalar_mult(const float *a, float u, float *out)
{
  __m128 x[] ={ _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };
  __m128 y = _mm_load1_ps(&u);

  _mm_store_ps(out+0, _mm_mul_ps(x[0], y));
  _mm_store_ps(out+4, _mm_mul_ps(x[1], y));
  _mm_store_ps(out+8, _mm_mul_ps(x[2], y));
  _mm_store_ps(out+12, _mm_mul_ps(x[3], y));
}

void vec_dot(const float *a, const float *b, float *out)
{
  __m128 dot = dot_ps(_mm_load_ps(a), _mm_load_ps(b));

  _mm_store_ss(out, dot);
}

void vec_normalize(const float *a, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 l2 = dot_ps(x, x);

  x = _mm_mul_ps(x, _mm_rsqrt_ps(l2));  // x = x * (1.0f / x.length())

  _mm_store_ps(out, x);
}

void vec_recip(const float *a, float *out)
{
  __m128 x = _mm_load_ps(a);

  x = _mm_rcp_ps(x);

  _mm_store_ps(out, x);
}

void vec_scalar_mult(const float *a, float u, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps1(&u);

  _mm_store_ps(out, _mm_mul_ps(x, y));
}

void vec4_scalar_recip_mult(const float *a, float u, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps1(&u);

  y = _mm_rcp_ps(y);
  x = _mm_mul_ps(x, y);

  _mm_store_ps(out, x);
}

void vec4_lerp(const float *a, const float *b, float u, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps(b);

  __m128 d = _mm_mul_ps(_mm_sub_ps(y, x), _mm_load_ps1(&u));

  _mm_store_ps(out, _mm_add_ps(x, d));
}

void vec3_cross(const float *a, const float *b, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps(b);

  __m128 z = cross_ps(x, y);

  _mm_store_ps(out, z);
}

void quat_mult(const float *a, const float *b, float *out)
{
  __m128 x = _mm_load_ps(a);

  __m128 x1 = shuffle_ps(x, W, Z, Y, X);
  __m128 x2 = shuffle_ps(x, Z, W, X, Y);
  __m128 x3 = shuffle_ps(x, Y, X, W, Z);

  x1 = neg_ps(x1, 1, 1, 1, 0);   // x1 = { -x, -y, -z, w }
  x3 = neg_ps(x3, 1, 1, 1, 1);   // x3 = -x3

  __m128 z;
  __m128 w = _mm_setzero_ps();

  z = _mm_mul_ps(x, _mm_load_ps1(b+3));
  w = _mm_add_ps(w, z);
  z = _mm_mul_ps(x1, _mm_load_ps1(b+0));
  w = _mm_add_ps(w, z);
  z = _mm_mul_ps(x2, _mm_load_ps1(b+1));
  w = _mm_add_ps(w, z);
  z = _mm_mul_ps(x3, _mm_load_ps1(b+2));
  w = _mm_add_ps(w, z);

  _mm_store_ps(out, w);
}

void quat_vec3_mult(const float *a, const float *b, float *out)
{
  __m128 u = _mm_load_ps(a);
  __m128 v = _mm_load_ps(b);

  __m128 s  = mask_xyz_ps(_mm_load_ps1(a+3));
  __m128 s2 = _mm_mul_ps(s, s);   // s^2
  __m128 ss = _mm_add_ps(s, s);   // 2*s

  __m128 z;
  __m128 e;

  e = dot_xyz_ps(u, v);
  e = _mm_add_ps(e, e);           // e = 2.0f*e

  z = _mm_mul_ps(u, e);  // z = u * 2*u.dot(v)

  e = dot_xyz_ps(u, u);
  e = _mm_sub_ps(s2, e);          // e = s*s - u.dot(u)
  e = _mm_mul_ps(v, e);

  z = _mm_add_ps(z, e);   // z = (u * 2*u.dot(v)) + v*(s*s - u.dot(u))

  e = cross_ps(u, v);
  e = _mm_mul_ps(e, ss);  // e = u.cross(v) * 2.0f*s

  // z = (u * 2*u.dot(v)) + v*(s*s - u.dot(u)) + (u.cross(v) * 2.0f*s)
  z = _mm_add_ps(z, e);

  _mm_store_ps(out, z);
}

void quat_to_mat4x3(const float *a, float *out)
{
  __m128 q = _mm_load_ps(a);
  __m128 qq = _mm_add_ps(q, q);

  __m128 p = _mm_mul_ps(shuffle_ps(q, Y, X, X, Y), shuffle_ps(qq, Y, Y, Z, Z));
  __m128 r = _mm_mul_ps(shuffle_ps(q, Z, W, W, W), shuffle_ps(qq, Z, Z, Y, X));
  p = neg_ps(p, 1, 0, 0, 0);    // p = { -yy2, xy2, xz2, yz2 }
  r = neg_ps(r, 0, 0, 1, 1);    // r = { zz2, wz2, -wy2, -wx2 }

  __m128 l = _mm_mul_ps(q, qq);
  l = neg_ps(l, 1, 0, 0, 0);    // l = { -xx2, yy2, zz2, ww2 }

  __m128 r_;
  __m128 m[3];

  r_ = neg_ps(r, 0, 1, 1, 0);
  m[0] = _mm_sub_ps(
    p,              // -yy2, xy2, xz2, yz2
    r_              // zz2, -wz2, wy2, -wx2
  );    // -yy2 - zz2, xy2 + wz2, xz2 - wy2

  m[1] = _mm_sub_ps(
    shuffle_ps(
      shuffle2_ps(p, l, Y, W, X, W),  // xy2, yz2, -xx2
      X, Z, Y, W),  // xy2, -xx2, yz2
    shuffle_ps(
      shuffle2_ps(r, l, Y, W, Z, W),  // wz2, -wx2, zz2
      X, Z, Y, W)   // wz2, zz2, -wx2
  );    // xy2 - wz2, -xx2 - zz2, yz2 + wx2

  r_ = neg_ps(r, 0, 0, 0, 1);  // zz2, wz2, -wy2, wx2
  m[2] = _mm_sub_ps(
    shuffle2_ps(p, l, Z, W, X, W),   // xz2, yz2, -xx2
    shuffle2_ps(r_, l, Z, W, Y, W)    // -wy2, wx2, yy2
  );

  m[0] = _mm_add_ps(m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f));
  m[1] = _mm_add_ps(m[1], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f));
  m[2] = _mm_add_ps(m[2], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));

  // Transpose 'm'
  __m128 t[2];
  t[0] = shuffle2_ps(m[0], m[1], X, Y, X, Y); // m(0, 0), m(0, 1), m(1, 0), m(1, 1)
  t[1] = shuffle2_ps(m[0], m[1], Z, W, Z, W); // m(0, 2), m(0, 3), m(1, 2), m(1, 3)
  //                                  m[2] =     m(2, 0), m(2, 1), m(2, 2), m(2, 3)

  m[0] = shuffle2_ps(t[0], m[2], X, Z, X, W); // m(0, 0), m(1, 0), m(2, 0)
  m[1] = shuffle2_ps(t[0], m[2], Y, W, Y, W); // m(0, 1), m(1, 1), m(2, 1)
  m[2] = shuffle2_ps(t[1], m[2], X, Z, Z, W); // m(0, 2), m(1, 2), m(2, 2)

  _mm_store_ps(out + (0*4), m[0]);
  _mm_store_ps(out + (1*4), m[1]);
  _mm_store_ps(out + (2*4), m[2]);
}

// Avoid global namespace pollution

#undef X
#undef Y
#undef Z
#undef W

#undef cross_ps
#undef dot_xyz_ps
#undef dot_ps
#undef mask_xyz_ps
#undef neg_ps
#undef broadcast_ps
#undef shuffle2_ps
#undef shuffle_ps
#undef pshufd
#undef epi32_ps
#undef ps_epi32

}