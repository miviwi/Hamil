#pragma once

#include <math/geometry.h>

namespace xform {

class Transform {
public:
  Transform();
  Transform(const mat4& m_);

  Transform& translate(float x, float y, float z);
  Transform& translate(vec3 pos);
  Transform& translate(vec4 pos);
  Transform& scale(float x, float y, float z);
  Transform& scale(float s);
  Transform& scale(vec3 s);
  Transform& rotx(float angle);
  Transform& roty(float angle);
  Transform& rotz(float angle);

  Transform& transform(const mat4& t);

  const mat4& matrix() const;

private:
  mat4 m;
};

}