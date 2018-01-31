#include <math/geometry.h>

#include <xmmintrin.h>
#include <pmmintrin.h>

namespace intrin {

void mat4_mult(const float *a, const float *b, float *out)
{
  __m128 x[] = { _mm_load_ps(b+0), _mm_load_ps(b+4), _mm_load_ps(b+8), _mm_load_ps(b+12) };

  __m128 y0, y1, y2, y3;
  __m128 z0, z1, z2, z3;

  z0 = _mm_mul_ps(_mm_load_ps1(a+0), x[0]);
  z1 = _mm_mul_ps(_mm_load_ps1(a+4), x[0]);
  z2 = _mm_mul_ps(_mm_load_ps1(a+8), x[0]);
  z3 = _mm_mul_ps(_mm_load_ps1(a+12), x[0]);

  for(int i = 1; i < 4; i++) {
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
  __m128 x[] = { _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };

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

  det = tmp = _mm_set1_ps(0.0f);

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
  __m128 xa[] = { _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };
  __m128 xb[] = {
    _mm_shuffle_ps(_mm_unpacklo_ps(xa[0], xa[1]), _mm_unpacklo_ps(xa[2], xa[3]), 0x44),
    _mm_shuffle_ps(_mm_unpacklo_ps(xa[0], xa[1]), _mm_unpacklo_ps(xa[2], xa[3]), 0xEE),
    _mm_shuffle_ps(_mm_unpackhi_ps(xa[0], xa[1]), _mm_unpackhi_ps(xa[2], xa[3]), 0x44),
    _mm_shuffle_ps(_mm_unpackhi_ps(xa[0], xa[1]), _mm_unpackhi_ps(xa[2], xa[3]), 0xEE),
  };

  __m128 y;
  __m128 z;

  z = _mm_mul_ps(_mm_load1_ps(b), xb[0]);

  for(int i = 1; i < 4; i++) {
    y = _mm_mul_ps(_mm_load1_ps(b+i), xb[i]);

    z = _mm_add_ps(y, z);
  }

  _mm_store_ps(out, z);
}

void vec4_const_mult(const float *a, float u, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps1(&u);

  _mm_store_ps(out, _mm_mul_ps(x, y));
}

void vec4_lerp(const float *a, const float *b, float u, float *out)
{
  __m128 x = _mm_load_ps(a);
  __m128 y = _mm_load_ps(b);

  __m128 d = _mm_sub_ps(y, x);

  d = _mm_mul_ps(d, _mm_load_ps1(&u));
  d = _mm_add_ps(x, d);

  _mm_store_ps(out, d);
}

}