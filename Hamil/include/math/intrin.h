#pragma once

#include <common.h>

namespace intrin {

INTRIN_INLINE void vec_dot(const float *a, const float *b, float *out);
INTRIN_INLINE void vec_normalize(const float *a, float *out);
INTRIN_INLINE void vec_recip(const float *a, float *out);

INTRIN_INLINE void vec3_cross(const float *a, const float *b, float *out);

INTRIN_INLINE void vec4_const_mult(const float *a, float u, float *out);
INTRIN_INLINE void vec4_const_recip_mult(const float *a, float u, float *out);
INTRIN_INLINE void vec4_lerp(const float *a, const float *b, float u, float *out);

INTRIN_INLINE void mat4_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void mat4_transpose(const float *a, float *out);
INTRIN_INLINE void mat4_inverse(const float *a, float *out);
INTRIN_INLINE void mat4_vec4_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void mat4_const_mult(const float *a, float u, float *out);

INTRIN_INLINE void quat_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void quat_vec3_mult(const float *a, const float *b, float *out);
INTRIN_INLINE void quat_to_mat4x3(const float *a, float *out);

}

#include <math/intrin.hh>