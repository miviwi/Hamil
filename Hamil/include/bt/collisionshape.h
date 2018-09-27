#pragma once

#include <bt/bullet.h>

namespace bt {

// Non-owning wrapper around btCollisionShape
class CollisionShape {
public:
  CollisionShape();

  // Returns the underlying handle
  //   to a btCollisionShape as a void *
  void *get() const;

protected:
  CollisionShape(btCollisionShape *shape);

private:
  friend class DynamicsWorld;
  friend class RigidBody;

  btCollisionShape *m;
};

}