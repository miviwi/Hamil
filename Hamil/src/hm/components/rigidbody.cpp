#include <hm/components/rigidbody.h>

#include <bt/btcommon.h>

#include <cstdio>

namespace hm {

RigidBody::RigidBody(u32 entity, bt::RigidBody rb_) :
  Component(entity),
  rb(rb_)
{
  rb.user(entity);
}

void RigidBody::destroyed()
{
}

}