#include <bt/world.h>
#include <bt/collisionshape.h>
#include <bt/ray.h>
#include <bt/btcommon.h>

#include <math/geometry.h>

namespace bt {

class DynamicsWorldConfig {
public:
  ~DynamicsWorldConfig();

  btCollisionConfiguration *m_collision_config;
  btDispatcher             *m_collision_dispatch;
  btBroadphaseInterface    *m_collision_broadphase;
  btConstraintSolver       *m_collision_solver;

  static DynamicsWorldConfig *create_default();

  btDynamicsWorld *discreteWorldFromConfig();

protected:
  DynamicsWorldConfig() = default;

private:
  friend DynamicsWorld;
};

DynamicsWorldConfig::~DynamicsWorldConfig()
{
  delete m_collision_solver;
  delete m_collision_broadphase;
  delete m_collision_dispatch;
  delete m_collision_config;
}

DynamicsWorldConfig *DynamicsWorldConfig::create_default()
{
  auto self = new DynamicsWorldConfig();

  self->m_collision_config     = new btDefaultCollisionConfiguration();
  self->m_collision_dispatch   = new btCollisionDispatcher(self->m_collision_config);
  self->m_collision_broadphase = new btDbvtBroadphase();
  self->m_collision_solver     = new btSequentialImpulseConstraintSolver();

  return self;
}

btDynamicsWorld *DynamicsWorldConfig::discreteWorldFromConfig()
{
  return new btDiscreteDynamicsWorld(
    m_collision_dispatch, m_collision_broadphase, m_collision_solver, m_collision_config
  );
}

DynamicsWorld::DynamicsWorld()
{
  m_config = DynamicsWorldConfig::create_default();
  m_world  = m_config->discreteWorldFromConfig();
}

DynamicsWorld::~DynamicsWorld()
{
  if(deref()) return;

  foreachObject([this](btCollisionObject *obj) {
    auto rb = btRigidBody::upcast(obj);
    if(rb) {
      if(auto motion_state = rb->getMotionState()) delete motion_state;
    }

    m_world->removeCollisionObject(obj);
    delete obj;
  });

  delete m_world;
  delete m_config;
}

void *DynamicsWorld::get() const
{
  return m_world;
}

void DynamicsWorld::addRigidBody(RigidBody rb)
{
  m_world->addRigidBody(rb.m);
}

void DynamicsWorld::removeRigidBody(RigidBody rb)
{
  m_world->removeRigidBody(rb.m);
}

RayClosestHit DynamicsWorld::rayTestClosest(Ray ray)
{
  auto from = to_btVector3(ray.m_from);
  auto to   = to_btVector3(ray.m_to);

  btCollisionWorld::ClosestRayResultCallback callback(from, to);

  m_world->rayTest(from, to, callback);
  if(callback.hasHit()) {
    RayClosestHit hit;

    hit.m_collision_object = callback.m_collisionObject;

    hit.m_hit_point  = from_btVector3(callback.m_hitPointWorld);
    hit.m_hit_normal = from_btVector3(callback.m_hitNormalWorld);

    return hit;
  }

  return RayClosestHit();
}

void DynamicsWorld::step(float dt)
{
  m_world->stepSimulation(dt, SimulationMaxSubsteps);
}

static CollisionShape p_sphere;

void DynamicsWorld::initDbgSimulation()
{
  m_world->setGravity({ 0.0f, -10.0f, 0.0f });
  {
    auto ground_shape = shapes().box({ 50.0f, 0.5f, 50.0f, });
    vec3 origin = { 0.0f, -1.5f, -6.0f };

    auto body = RigidBody::create(ground_shape, origin, 0.0f, false);
    body.rollingFriction(0.2f);

    addRigidBody(body);
  }

  p_sphere = shapes().sphere(1.0f);
}

void DynamicsWorld::startDbgSimulation()
{
  foreachRigidBody([this](btRigidBody *rb) {
    rb->activate();
    rb->forceActivationState(ACTIVE_TAG);
  });
}

RigidBody DynamicsWorld::createDbgSimulationRigidBody(vec3 sphere, bool active)
{
  return RigidBody::create(p_sphere, sphere, 1.0f, active);
}

void DynamicsWorld::foreachObject(BtCollisionObjectIter fn)
{
  auto& objects = m_world->getCollisionObjectArray();
  for(auto i = objects.size()-1; i >= 0; i--) {
    auto obj = objects[i];
    fn(obj);
  }
}

void DynamicsWorld::foreachRigidBody(BtRigidBodyIter fn)
{
  foreachObject([&](btCollisionObject *obj) {
    if(auto rb = btRigidBody::upcast(obj)) {
      fn(rb);
    }
  });
}

}