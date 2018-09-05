#pragma once

#include <math/geometry.h>

struct Quaternion;

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

  // Applies the transformation encoded by 't'
  //   (pre-multiplies the underlying matrix by it)
  Transform& transform(const mat4& t);

  // Returns the underlying matrix
  const mat4& matrix() const;

  vec3 translation() const;

  // Returns the diagonal components of the underlying matrix
  vec3 scale() const;

  // Extracts the orientation from the upper-left 3x3 part of
  //   the underlying matrix via Quaternion::from_mat4()
  // Will break with non-uniform scales/shears
  Quaternion orientation() const;

private:
  mat4 m;
};

}