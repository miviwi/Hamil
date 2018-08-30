#include <bt/world.h>
#include <bt/btcommon.h>

#include <math/geometry.h>

namespace bt {

DynamicsWorld::DynamicsWorld()
{
  m_collision_config     = new btDefaultCollisionConfiguration();
  m_collision_dispatch   = new btCollisionDispatcher(m_collision_config);
  m_collision_broadphase = new btDbvtBroadphase();
  m_collision_solver     = new btSequentialImpulseConstraintSolver();
  
  m_world = new btDiscreteDynamicsWorld(
    m_collision_dispatch, m_collision_broadphase, m_collision_solver, m_collision_config
  );
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

  delete m_collision_solver;
  delete m_collision_broadphase;
  delete m_collision_dispatch;
  delete m_collision_config;
}

void DynamicsWorld::addRigidBody(RigidBody rb)
{
  m_world->addRigidBody(rb.m);
}

void DynamicsWorld::removeRigidBody(RigidBody rb)
{
  m_world->removeRigidBody(rb.m);
}

static btAlignedObjectArray<btCollisionShape *> shapes;

void DynamicsWorld::initDbgSimulation()
{
  m_world->setGravity({ 0.0f, -10.0f, 0.0f });
  {
    auto ground_shape = new btBoxShape({ 50.0f, 0.5f, 50.0f, });
    shapes.push_back(ground_shape);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin({ 0.0f, -1.5f, -6.0f });

    auto rb_info      = btRigidBody::btRigidBodyConstructionInfo(
      0.0f, nullptr, ground_shape
    );
    auto body = new btRigidBody(rb_info);
    body->setWorldTransform(transform);
    body->setActivationState(DISABLE_SIMULATION);

    m_world->addRigidBody(body);
  }

  {
    auto sphere_shape = new btSphereShape(1.0f);
    shapes.push_back(sphere_shape);

    std::vector<vec3> spheres = {
      { 2.0f, 0.0f, 0.0f },
      { 2.5f, 32.0f, 0.0f },
      { 2.0f, 32.0f, -0.5f },
      { 2.0f, 30.0f, -4.0f },
      { 2.5f, 32.0f, -4.0f },
      { 2.0f, 32.0f, -4.5f },

      { 2.0f, 10.0f, 0.0f },
      { 2.5f, 12.0f, 0.0f },
      { 2.0f, 12.0f, -0.5f },
      { 2.0f, 10.0f, -4.0f },
      { 2.5f, 12.0f, -4.0f },
      { 2.0f, 12.0f, -4.5f },
    };

    for(auto sphere : spheres) {
      auto body = createDbgSimulationRigidBody(sphere, false);

      addRigidBody(body);
    }
  }

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
  auto sphere_shape = shapes[1];

  btTransform transform;
  transform.setIdentity();

  btScalar  mass          = 1.0f;
  btVector3 local_inertia = { 0.0f, 0.0f, 0.0f };

  sphere_shape->calculateLocalInertia(mass, local_inertia);

  transform.setOrigin(to_btVector3(sphere));

  auto motion_state = new btDefaultMotionState(transform);
  auto rb_info      = btRigidBody::btRigidBodyConstructionInfo(
    mass, motion_state, sphere_shape, local_inertia
  );
  auto body = new btRigidBody(rb_info);
  if(!active) body->setActivationState(DISABLE_SIMULATION);

  return body;
}

void DynamicsWorld::stepDbgSimulation(float dt, RigidBodyIter fn) 
{
  m_world->stepSimulation(dt, SimulationMaxSubsteps);

  foreachRigidBody([&](btRigidBody *rb) {
    fn({ rb });
  });
}

RigidBody DynamicsWorld::pickDbgSimulation(vec3 ray_from, vec3 ray_to)
{
  auto from = to_btVector3(ray_from);
  auto to   = to_btVector3(ray_to);

  btCollisionWorld::ClosestRayResultCallback callback(from, to);

  m_world->rayTest(from, to, callback);
  if(callback.hasHit()) {
    return (btRigidBody *)btRigidBody::upcast(callback.m_collisionObject);
  }

  return nullptr;
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