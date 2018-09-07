#pragma once

#include <hm/component.h>

#include <bt/rigidbody.h>

namespace hm {

// !$Component
struct RigidBody : public Component {
  RigidBody(u32 entity, bt::RigidBody rb_) :
    Component(entity),
    rb(rb_)
  { }

  bt::RigidBody rb;
};

}
