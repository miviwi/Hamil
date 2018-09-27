#pragma once

#include <hm/component.h>

#include <bt/rigidbody.h>
#include <bt/collisionshape.h>

namespace hm {

// After creating the component it's parent entity
//   becomes accessible via rb->user<Entity>()
struct RigidBody : public Component {
  RigidBody(u32 entity, bt::RigidBody rb_);

  bt::RigidBody      rb;
  bt::CollisionShape shape;

  void destroyed();
};

}
