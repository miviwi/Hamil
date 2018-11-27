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
  RigidBody& forceActivate();
  RigidBody& deactivate();

  xform::Transform worldTransform() const;
  mat4 worldTransformMatrix() const;

  vec3 origin() const;
  vec3 localInertia() const;
  vec3 centerOfMass() const;
  float mass() const;

  AABB aabb() const;

  float rollingFriction() const;
  RigidBody& rollingFriction(float friction);

  void applyImpulse(const vec3& force, const vec3& rel_pos);

  RigidBody& createMotionState(const xform::Transform& t = xform::Transform());
  bool hasMotionState() const;

  // Returns 'true' for objects with non-zero mass
  bool isStatic() const;

  CollisionShape collisionShape() const;

  operator bool() const;

  bool operator==(const RigidBody& other) const;
  bool operator!=(const RigidBody& other) const;

  // Allocates a new btRigidBody with a btDefaultMotionState when mass > 0.0f
  //   i.e. when the body isn't static
  static RigidBody create(CollisionShape shape, vec3 origin, float mass = 0.0f, bool active = true);

protected:
  RigidBody(btRigidBody *m_);

  btMotionState *getMotionState() const;
  btTransform getWorldTransform() const;

private:
  friend class DynamicsWorld;
  friend class RayClosestHit;

  btRigidBody *m;
};

}
