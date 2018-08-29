#pragma once

#include <bt/bullet.h>

#include <math/geometry.h>
#include <math/transform.h>

namespace bt {

class RigidBody {
public:
  xform::Transform worldTransform() const;
  mat4 worldTransformMatrix() const;

  vec3 origin() const;
  vec3 localInertia();

  bool hasMotionState() const;

protected:
  RigidBody(btRigidBody *m_);

  btMotionState *getMotionState() const;
  btTransform getWorldTransform() const;

private:
  friend class DynamicsWorld;

  btRigidBody *m;
};

}
