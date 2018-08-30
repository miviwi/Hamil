#include <bt/rigidbody.h>
#include <bt/btcommon.h>

namespace bt {

RigidBody::RigidBody(btRigidBody *m_) :
  m(m_)
{ }

RigidBody::operator bool() const
{
  return m;
}

bool RigidBody::operator==(const RigidBody& other) const
{
  return m == other.m;
}

bool RigidBody::operator!=(const RigidBody& other) const
{
  return m != other.m;
}

RigidBody::RigidBody() :
  m(nullptr)
{
}

void *RigidBody::get() const
{
  return m;
}

xform::Transform RigidBody::worldTransform() const
{
  auto transform = getWorldTransform();

  mat4 matrix;
  transform.getOpenGLMatrix(matrix);

  return { matrix.transpose() };
}

mat4 RigidBody::worldTransformMatrix() const
{
  return worldTransform().matrix();
}

vec3 RigidBody::origin() const
{
  return from_btVector3(getWorldTransform().getOrigin());
}

vec3 RigidBody::localInertia()
{
  return from_btVector3(m->getLocalInertia());
}

bool RigidBody::hasMotionState() const
{
  return getMotionState();
}

btMotionState *RigidBody::getMotionState() const
{
  return m->getMotionState();
}

btTransform RigidBody::getWorldTransform() const
{
  if(auto motion_state = getMotionState()) {
    btTransform transform;
    motion_state->getWorldTransform(transform);

    return transform;
  }

  return m->getWorldTransform();
}

}