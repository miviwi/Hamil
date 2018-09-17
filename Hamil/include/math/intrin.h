#pragma once

namespace intrin {

void vec3_cross(const float *a, const float *b, float *out);

void vec4_const_mult(const float *a, float u, float *out);
void vec4_lerp(const float *a, const float *b, float u, float *out);
void vec4_recip(const float *a, float *out);

void mat4_mult(const float *a, const float *b, float *out);
void mat4_transpose(const float *a, float *out);
void mat4_inverse(const float *a, float *out);
void mat4_vec4_mult(const float *a, const float *b, float *out);

void quat_mult(const float *a, const float *b, float *out);
void quat_vec3_mult(const float *a, const float *b, float *out);

}