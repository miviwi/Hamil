#pragma once

#include <common.h>

namespace intrin {

using half = u16;

INTRIN_INLINE void vec_dot(const float *a, const float *b, float *out);
INTRIN_INLINE void vec_normalize(const float *a, float *out);
INTRIN_INLINE void vec_recip(const float *a, float *out);
INTRIN_INLINE void vec_scalar_mult(const float *a, float u, float *out);

INTRIN_INLINE void vec3_cross(const float *a, const float *b, float *out);

INTRIN_INLINE void vec4_scalar_recip_mult(const float *a, float u, float *out);
INTRIN_INLINE void vec4_lerp(const float *a, const float *b, float u, float *out);

INTRIN_INLINE void mat4_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void mat4_transpose(const float *a, float *out);
INTRIN_INLINE void mat4_inverse(const float *a, float *out);
INTRIN_INLINE void mat4_vec4_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void mat4_scalar_mult(const float *a, float u, float *out);

INTRIN_INLINE void quat_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void quat_vec3_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void quat_to_mat4x3(const float *a, float *out);

// Converts 4 float elements pointed to by 'src' to
//   half-precision and writes them to 'dst'
INTRIN_INLINE void stream4_f16(const float *src, half *dst);

// Converts 4 half-precision values pointed to by 'src' to
//   single-precision and writes them to 'dst'
INTRIN_INLINE void load4_f16(const half *src, float *dst);

}

#include <math/intrin.hh>