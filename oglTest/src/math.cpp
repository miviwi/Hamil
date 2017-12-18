#include "math.h"

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

float mat4_det(const float *a)
{
  __m128 xa[] = { _mm_load_ps(a+0), _mm_load_ps(a+4), _mm_load_ps(a+8), _mm_load_ps(a+12) };

  return 0;
}

}