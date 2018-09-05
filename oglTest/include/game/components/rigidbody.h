#pragma once

#include <game/component.h>

#include <bt/rigidbody.h>

namespace game {

// !$Component
struct RigidBody : public Component {
  RigidBody(u32 entity, bt::RigidBody rb_) :
    Component(entity),
    rb(rb_)
  { }

  bt::RigidBody rb;
};

}
