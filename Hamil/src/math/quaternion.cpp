#include <math/quaternion.h>

Quaternion Quaternion::from_axis(vec3 axis, float angle)
{
  float half_angle = angle * 0.5f;
  float s = sinf(half_angle);

  axis *= s;

  return { axis.x, axis.y, axis.z, cosf(half_angle) };
}

Quaternion Quaternion::from_euler(float x, float y, float z)
{
  x *= 0.5f; y *= 0.5f; z *= 0.5f;

  float cx = cos(x),
    cy = cos(y),
    cz = cos(z);

  float sx = sin(x),
    sy = sin(y),
    sz = sin(z);

  float cycz = cy*cz,
    sysz = sy*sz,
    cysz = cy*sz,
    sycz = sy*cz;

  Quaternion q = {
    sx*cycz - cx*sysz,
    cx*sycz + sx*cysz,
    cx*cysz - sx*sycz,
    cx*cycz + sx*sysz
  };

  return q.normalize();
}

// Source: https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
Quaternion Quaternion::from_mat3(const mat3& m)
{
  Quaternion q;
  float t;

  if(m(2, 2) < 0.0f) {
    if(m(0, 0) > m(1, 1)) {
      t = 1.0f + m(0, 0) - m(1, 1) - m(2, 2);
      q = { t, m(1, 0)+m(0, 1), m(0, 2)+m(2, 0), m(2, 1)-m(1, 2) };
    } else {
      t = 1.0f - m(0, 0) + m(1, 1) - m(2, 2);
      q = { m(1, 0)+m(0, 1), t, m(2, 1)+m(1, 2), m(0, 2)-m(2, 0) };
    }
  } else {
    if(m(0, 0) < -m(1, 1)) {
      t = 1.0f - m(0, 0) - m(1, 1) + m(2, 2);
      q = { m(0, 2)+m(2, 0), m(2, 1)+m(1, 2), t, m(1, 0)-m(0, 1) };
    } else {
      t = 1.0f + m(0, 0) + m(1, 1) + m(2, 2);
      q = { m(2, 1)-m(1, 2), m(0, 2)-m(2, 0), m(1, 0)-m(0, 1), t };
    }
  }

  t = 0.5f / sqrtf(t);

  return q * t;
}

Quaternion Quaternion::from_mat4(const mat4& m)
{
  return from_mat3(m.xyz());
}

// Source: https://github.com/OGRECave/ogre/blob/master/OgreMain/include/OgreVector.h
Quaternion Quaternion::rotation_between(const vec3& u, const vec3& v)
{
  Quaternion q;
  vec3 axis;

  float s = sqrtf(u.length2() * v.length2());
  float w = s + u.dot(v);

  if(w < (s * 1e-6f)) {
    w = 0.0f;

    if(u.x > u.z) {
      axis = { -u.y, u.x, 0.0f };
    } else {
      axis = { 0.0f, -u.z, u.y };
    }
  } else {
    axis = u.cross(v);
  }

  return Quaternion(axis, w).normalize();
}

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t)
{
  Quaternion q;
  float c = a.dot(b);

  if(c < 0.0f) {
    c = -c;
    q = -b;
  } else {
    q = b;
  }

  if(c < (1.0f - 1e-3f)) {    // Slerp
    float s = sqrtf(1.0f - c*c);
    float alpha = atan2(s, c);

    float inv_s = 1.0f / s;

    float p = sin((1.0f - t)*alpha) * inv_s;
    float r = sin(t*alpha) * inv_s;

    return a*p + q*r;
  }

  // Linear interpolation fallback
  return Quaternion(a*(1.0f - t) + q*t).normalize();
}
