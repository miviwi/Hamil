#pragma once

#include <bt/bullet.h>

#include <math/geometry.h>
#include <math/transform.h>

namespace bt {

class RigidBody {
public:
  RigidBody();

  void *get() const;

  void activate();

  xform::Transform worldTransform() const;
  mat4 worldTransformMatrix() const;

  vec3 origin() const;
  vec3 localInertia() const;
  vec3 centerOfMass() const;

  void applyImpulse(const vec3& force, const vec3& rel_pos);

  bool hasMotionState() const;

  operator bool() const;

  bool operator==(const RigidBody& other) const;
  bool operator!=(const RigidBody& other) const;

protected:
  RigidBody(btRigidBody *m_);

  btMotionState *getMotionState() const;
  btTransform getWorldTransform() const;

private:
  friend class DynamicsWorld;

  btRigidBody *m;
};

}
