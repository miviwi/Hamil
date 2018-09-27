#pragma once

#include <bt/bullet.h>

#include <math/geometry.h>
#include <math/transform.h>

namespace bt {

class CollisionShape;

// Non-owning wrapper around btRigidBody
class RigidBody {
public:
  RigidBody();

  // Returns the underlying handle
  //   to a btRigidBody as a void *
  void *get() const;

  void *user() const;
  void user(void *ptr);

  // Returns the stored user() pointer as T
  template <typename T>
  T user() const
  {
    return T((uintptr_t)user());
  }

  // Stores the given T as a void *
  template <typename T>
  void user(T u)
  {
    user((void *)(uintptr_t)u);
  }

  RigidBody& activate();

  xform::Transform worldTransform() const;
  mat4 worldTransformMatrix() const;

  vec3 origin() const;
  vec3 localInertia() const;
  vec3 centerOfMass() const;

  void applyImpulse(const vec3& force, const vec3& rel_pos);

  bool hasMotionState() const;

  CollisionShape collisionShape() const;

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
