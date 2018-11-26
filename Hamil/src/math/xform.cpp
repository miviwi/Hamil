#include <math/xform.h>

namespace xform {

mat4 identity()
{
  return mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 translate(float x, float y, float z)
{
  return mat4{
    1.0f, 0.0f, 0.0f, x,
    0.0f, 1.0f, 0.0f, y,
    0.0f, 0.0f, 1.0f, z,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 translate(vec3 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

mat4 translate(vec4 pos)
{
  return translate(pos.x, pos.y, pos.z);
}

mat4 scale(float x, float y, float z)
{
  return mat4{
    x,    0.0f, 0.0f, 0.0f,
    0.0f, y,    0.0f, 0.0f,
    0.0f, 0.0f, z,    0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 scale(float s)
{
  return scale(s, s, s);
}

mat4 rotx(float a)
{
  float s = sinf(a),
    c = cosf(a);

  return mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, c,    -s,   0.0f,
    0.0f, s,    c,    0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 roty(float a)
{
  float s = sinf(a),
    c = cosf(a);

  return mat4{
    c,    0.0f, s,    0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    -s,   0.0f, c,    0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 rotz(float a)
{
  float s = sinf(a),
    c = cosf(a);

  return mat4{
    c,    -s,   0.0f, 0.0f,
    s,    c,    0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

mat4 ortho(float t, float l, float b, float r, float n, float f)
{
  return mat4{
    2.0f/(r-l), 0.0f,       0.0f,        -(r+l)/(r-l),
    0.0f,       2.0f/(t-b), 0.0f,        -(t+b)/(t-b),
    0.0f,       0.0f,       -2.0f/(f-n), -(f+n)/(f-n),
    0.0f,       0.0f,       0.0f,        1.0f,
  };
}

mat4 frutsum(float t, float l, float b, float r, float n, float f)
{
  auto nn = n*2.0f;
  return mat4{
    nn/(r-l), 0.0f,     (r+l)/(r-l),  0.0f,
    0.0f,     nn/(t-b), (t+b)/(t-b),  0.0f,
    0.0f,     0.0f,     -(f+n)/(f-n), -f*nn/(f-n),
    0.0f,     0.0f,     -1.0f,        0.0f,
  };
}

mat4 perspective(float fovy, float aspect, float n, float f)
{
  constexpr float inv_deg2rad = 1.0f / (360.0f*PIf);

  float w, h;
  h = tanf(fovy * inv_deg2rad) * n;
  w = h * aspect;

  return frutsum(h, -w, -h, w, n, f);
}

mat4 perspective_inf(float fovy, float aspect, float n)
{
  constexpr float inv_deg2rad = 1.0f / (360.0f*PIf);

  float w, h;
  h = tanf(fovy * inv_deg2rad) * n;
  w = h * aspect;

  float nn = n*2.0f;
  float t = h, l = -w, b = -h, r = w;

  return mat4{
    nn/(r-l), 0.0f,     (r+l)/(r-l), 0.0f,
    0.0f,     nn/(t-b), (t+b)/(t-b), 0.0f,
    0.0f,     0.0f,     -1.0f,       -nn,
    0.0f,     0.0f,     -1.0f,       0.0f,
  };
}

mat4 look_at(vec3 eye, vec3 target, vec3 up)
{
  vec3 forward = vec3::direction(target, eye);
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

vec2 project(vec4 v, mat4 modelviewprojection, ivec2 screen)
{
  v = modelviewprojection * v;
  v = v.perspectiveDivide();

  auto screenf = screen.cast<float>();

  return {
    (v.x+1.0f) * (1.0f/2.0f) * screenf.x,
    screenf.y - ((v.y+1.0f) * (1.0f/2.0f) * screenf.y)
  };
}

vec4 unproject(vec3 v, mat4 modelviewprojection, ivec2 screen)
{
  auto inv_mvp = modelviewprojection.inverse();
  auto inv_screen = screen.cast<float>().recip();

  vec4 p = {
    (v.x * inv_screen.x)*2.0f - 1.0f,
    (((float)screen.y - v.y) * inv_screen.y)*2.0f - 1.0f,
    2.0f*v.z - 1.0f,
    1.0f
  };

  p = inv_mvp * p;

  // Avoid divide by 0
  if(p.w == 0.0f) return { vec3(0.0f), 0.0f };

  return p.perspectiveDivide();
}

}