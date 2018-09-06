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
Quaternion Quaternion::rotation_between(const vec3& a, const vec3& b)
{
  Quaternion q;

  vec3 v0 = a.normalize(),
    v1 = b.normalize();

  auto d = v0.dot(v1);
  if(d >= 1.0f) {
    return Quaternion();
  } else if(d < (1e-6f - 1.0f)) {
    auto axis = vec3(1.0f, 0.0f, 0.0f).cross(a);
    if(axis.zeroLength()) axis = vec3(0.0f, 1.0f, 0.0f).cross(a);

    return Quaternion::from_axis(axis.normalize(), PIf);
  }

  float s = sqrtf((1.0f + d)*2.0f);
  float inv_s = 1.0f / s;

  auto c = v0.cross(v1) * inv_s;

  return Quaternion(c, s*0.5f).normalize();
}