#include <hm/components/rigidbody.h>

#include <bt/btcommon.h>

#include <cstdio>

namespace hm {

RigidBody::RigidBody(u32 entity, bt::RigidBody rb_) :
  Component(),
  rb(rb_), shape(rb_.collisionShape())
{
  rb.user(entity);
}

void RigidBody::destroyed()
{
}

}