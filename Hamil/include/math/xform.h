#pragma once

#include <math/geometry.h>

namespace xform {

mat4 identity();

mat4 translate(float x, float y, float z);
mat4 translate(vec3 pos);
mat4 translate(vec4 pos);

mat4 scale(float x, float y, float z);
mat4 scale(float s);

mat4 rotx(float a);
mat4 roty(float a);
mat4 rotz(float a);

mat4 ortho(float t, float l, float b, float r, float n, float f);
mat4 frutsum(float t, float l, float b, float r, float n, float f);
mat4 perspective(float fovy, float aspect, float n, float f);
mat4 perspective_inf(float fovy, float aspect, float n);

mat4 look_at(vec3 eye, vec3 target, vec3 up);

vec2 project(vec4 v, mat4 modelviewprojection, ivec2 screen);

// Returns a vec4 with w == 0 when the given coordinates couldn't be unprojected
vec4 unproject(vec3 v, mat4 modelviewprojection, ivec2 screen);

}