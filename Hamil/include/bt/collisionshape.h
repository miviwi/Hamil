#pragma once

#include <bt/bullet.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <vector>
#include <memory>

namespace bt {

// Non-owning wrapper around btCollisionShape
//   - the ref-count is only used by the CollisionShapeManager
class CollisionShape : public Ref {
public:
  CollisionShape();

  // Returns the underlying handle
  //   to a btCollisionShape as a void *
  void *get() const;

  const char *name() const;

  vec3 calculateLocalInertia(float mass);

protected:
  CollisionShape(btCollisionShape *shape);

  btCollisionShape *bt() const;

private:
  friend class DynamicsWorld;
  friend class RigidBody;
  friend class CollisionShapeManager;

  btCollisionShape *m;
};

class CollisionShapeManager {
public:
  enum {
    InitialAlloc = 256,
  };

  CollisionShapeManager();
  CollisionShapeManager(const CollisionShapeManager& other) = delete;
  ~CollisionShapeManager();

  CollisionShape box(vec3 half_extents);
  CollisionShape sphere(float radius);

private:
  std::vector<CollisionShape> m_shapes;
};

}