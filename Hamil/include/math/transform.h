#pragma once

#include <math/geometry.h>

struct Quaternion;

namespace xform {

class Transform {
public:
  Transform();
  Transform(const mat4& m_);
  Transform(vec3 position_);
  Transform(vec3 position_, Quaternion orientation_, vec3 scale_ = vec3(1.0f));

  Transform& translate(float x, float y, float z);
  Transform& translate(const vec3& pos);
  Transform& translate(const vec4& pos);
  Transform& scale(float x, float y, float z);
  Transform& scale(float s);
  Transform& scale(const vec3& s);
  Transform& rotx(float angle);
  Transform& roty(float angle);
  Transform& rotz(float angle);
  Transform& rotate(const Quaternion& q);

  // Applies the transformation encoded by 't'
  //   (pre-multiplies the underlying matrix by it)
  Transform& transform(const mat4& t);

  Transform& transform(const Transform& t);

  // Returns the underlying matrix
  const mat4& matrix() const;

  vec3 translation() const;
  vec3 scale() const;
  mat3 rotation() const;

  // Extracts the orientation from the upper-left 3x3 part of
  //   the underlying matrix via Quaternion::from_mat3()
  // Will break with shears/reflections
  Quaternion orientation() const;

private:
  mat4 m;
};

}