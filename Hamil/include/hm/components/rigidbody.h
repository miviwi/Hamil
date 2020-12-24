#pragma once

#include <hm/component.h>

#include <bt/rigidbody.h>
#include <bt/collisionshape.h>

namespace hm {

// After creating the component it's parent entity
//   becomes accessible via rb->user<Entity>()
struct RigidBody : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */, bt::RigidBody
  >;

  RigidBody(u32 entity, bt::RigidBody rb_);

  static const Tag tag() { return "RigidBody"; }

  bt::RigidBody      rb;
  bt::CollisionShape shape;

  void destroyed();
};

}
