#pragma once

#include <math/geometry.h>

namespace xform {

static mat4 identity()
{
  return mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 translate(float x, float y, float z)
{
  return mat4{
    1.0f, 0.0f, 0.0f, x,
    0.0f, 1.0f, 0.0f, y,
    0.0f, 0.0f, 1.0f, z,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 translate(vec3 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

static mat4 translate(vec4 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

static mat4 scale(float x, float y, float z)
{
  return mat4{
    x,    0.0f, 0.0f, 0.0f,
    0.0f, y,    0.0f, 0.0f,
    0.0f, 0.0f, z,    0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static mat4 scale(float s)
{
  return scale(s, s, s);
}

static mat4 rotx(float a)
{
  return mat4{
    1.0f, 0.0f,    0.0f,     0.0f,
    0.0f, cosf(a), -sinf(a), 0.0f,
    0.0f, sinf(a), cosf(a),  0.0f,
    0.0f, 0.0f,    0.0f,     1.0f,
  };
}

static mat4 roty(float a)
{
  return mat4{
    cosf(a), 0.0f, sinf(a), 0.0f,
    0.0f,     1.0f, 0.0f,    0.0f,
    -sinf(a), 0.0f, cosf(a), 0.0f,
    0.0f,     0.0f, 0.0f,    1.0f,
  };
}

static mat4 rotz(float a)
{
  return mat4{
    cosf(a), -sinf(a), 0.0f, 0.0f,
    sinf(a), cosf(a),  0.0f, 0.0f,
    0.0f,    0.0f,     1.0f, 0.0f,
    0.0f,    0.0f,     0.0f, 1.0f,
  };
}

static mat4 ortho(float t, float l, float b, float r, float n, float f)
{
  return mat4{
    2.0f/(r-l), 0.0f,       0.0f,        -(r+l)/(r-l),
    0.0f,       2.0f/(t-b), 0.0f,        -(t+b)/(t-b),
    0.0f,       0.0f,       -2.0f/(f-n), -(f+n)/(f-n),
    0.0f,       0.0f,       0.0f,        1.0f,
  };
}

static mat4 frutsum(float t, float l, float b, float r, float n, float f)
{
  auto n2 = n*2.0f;
  return mat4{
    n2/(r-l), 0.0f,     (r+l)/(r-l),  0.0f,
    0.0f,     n2/(t-b), (t+b)/(t-b),  0.0f,
    0.0f,     0.0f,     -(f+n)/(f-n), -f*n2/(f-n),
    0.0f,     0.0f,     -1.0f,        0.0f,

  };
}

static mat4 perspective(float fovy, float aspect, float n, float f)
{
  float w, h;

  h = tanf(fovy / (360.0f*PIf)) * n;
  w = h * aspect;

  return frutsum(h, -w, -h, w, n, f);
}

static mat4 look_at(vec3 eye, vec3 target, vec3 up)
{
  vec3 forward = (eye-target).normalize();
  vec3 left = up.cross(forward).normalize();

  up = forward.cross(left);

  float x = -left.x*eye.x - left.y*eye.y - left.z*eye.z,
    y = -up.x*eye.x - up.y*eye.y - up.z*eye.z,
    z = -forward.x*eye.x - forward.y*eye.y - forward.z*eye.z;

  return mat4{
    left.x,    left.y,    left.z,    x,
    up.x,      up.y,      up.z,      y,
    forward.x, forward.y, forward.z, z,
    0.0f,      0.0f,      0.0f,      1.0f,
  };

}

static vec2 project(vec4 v, mat4 modelviewprojection, ivec2 screen)
{
  v = modelviewprojection * v;
  v *= 1.0f/v.w;

  auto screenf = screen.cast<float>();

  return {
    (v.x+1)/2 * screenf.x,
    screenf.y - ((v.y+1)/2 * screenf.y)
  };
}

// Returns a vec4 with w == 0 when the given coordinates couldn't be unprojected
static vec4 unproject(vec3 v, mat4 modelviewprojection, ivec2 screen)
{
  auto inv_mvp = modelviewprojection.inverse();
  auto inv_screen = screen.cast<float>().recip();

  vec4 p ={
    (v.x * inv_screen.x)*2.0f - 1.0f,
    (((float)screen.y - v.y) * inv_screen.y)*2.0f - 1.0f,
    2.0f*v.z - 1.0f,
    1.0f
  };

  p = inv_mvp * p;
  if(p.w == 0.0f) return { vec3(0.0f), 0.0f };

  p.w = 1.0f/p.w;

  return { p.xyz() * p.w, 1.0f };
}

}