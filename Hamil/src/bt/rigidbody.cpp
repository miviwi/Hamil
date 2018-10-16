#include <bt/rigidbody.h>
#include <bt/collisionshape.h>
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

RigidBody RigidBody::create(CollisionShape shape, vec3 origin, float mass, bool active)
{
  btVector3 local_inertia = { 0.0f, 0.0f, 0.0f };

  btTransform transform;
  transform.setIdentity();
  transform.setOrigin(to_btVector3(origin));

  btMotionState *motion_state = nullptr;

  if(mass > 0.0f) {
    // The body is non-static so calculate the inertia
    //   and create a btMotionState for it

    local_inertia = to_btVector3(shape.calculateLocalInertia(mass));
    motion_state = new btDefaultMotionState(transform);
  }

  RigidBody body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
    mass, motion_state, shape.bt(), local_inertia
  ));

  // If we haven't created a btMotionState for this body
  //   set the world transform directly
  if(!motion_state) body.m->setWorldTransform(transform);

  if(!active) body.deactivate();

  return body;
}

RigidBody::RigidBody() :
  m(nullptr)
{
}

void *RigidBody::get() const
{
  return m;
}

void *RigidBody::user() const
{
  return m->getUserPointer();
}

void RigidBody::user(void *ptr)
{
  m->setUserPointer(ptr);
}

RigidBody& RigidBody::activate()
{
  m->activate();
  return *this;
}

RigidBody& RigidBody::forceActivate()
{
  activate();
  m->forceActivationState(ACTIVE_TAG);

  return *this;
}

RigidBody& RigidBody::deactivate()
{
  m->setActivationState(DISABLE_SIMULATION);
  return *this;
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

vec3 RigidBody::localInertia() const
{
  return from_btVector3(m->getLocalInertia());
}

vec3 RigidBody::centerOfMass() const
{
  return from_btVector3(m->getCenterOfMassPosition());
}

float RigidBody::rollingFriction() const
{
  return m->getRollingFriction();
}

RigidBody& RigidBody::rollingFriction(float friction)
{
  m->setRollingFriction(friction);
  return *this;
}

void RigidBody::applyImpulse(const vec3& force, const vec3& rel_pos)
{
  m->applyImpulse(to_btVector3(force), to_btVector3(rel_pos));
}

bool RigidBody::hasMotionState() const
{
  return getMotionState();
}

CollisionShape RigidBody::collisionShape() const
{
  return CollisionShape(m->getCollisionShape());
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