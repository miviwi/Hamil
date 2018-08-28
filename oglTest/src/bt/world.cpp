#include <bt/world.h>

#include <math/geometry.h>

#pragma warning(push, 0)      // Silence some harmless warnings
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#pragma warning(pop)

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

void DynamicsWorld::runDbgSimulation()
{
  m_world->setGravity({ 0.0f, -10.0f, 0.0f });

  btAlignedObjectArray<btCollisionShape *> shapes;
  {
    auto ground_shape = new btBoxShape({ 50.0f, 50.0f, 50.0f, });
    shapes.push_back(ground_shape);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin({ 0.0f, -56.0f, 0.0f });

    auto motion_state = new btDefaultMotionState(transform);
    auto rb_info      = btRigidBody::btRigidBodyConstructionInfo(
      0.0f, motion_state, ground_shape
    );
    auto body = new btRigidBody(rb_info);

    m_world->addRigidBody(body);
  }

  {
    auto sphere_shape = new btSphereShape(1.0f);
    shapes.push_back(sphere_shape);

    btTransform transform;
    transform.setIdentity();

    btScalar  mass          = 1.0f;
    btVector3 local_inertia = { 0.0f, 0.0f, 0.0f };

    sphere_shape->calculateLocalInertia(mass, local_inertia);

    transform.setOrigin({ 2.0f, 10.0f, 0.0f });

    auto motion_state = new btDefaultMotionState(transform);
    auto rb_info      = btRigidBody::btRigidBodyConstructionInfo(
      mass, motion_state, sphere_shape, local_inertia
    );
    auto body = new btRigidBody(rb_info);

    m_world->addRigidBody(body);
  }

  for(int i = 0; i < 10; i++) {
    m_world->stepSimulation(SimulationStep, SimulationMaxSubsteps);

    foreachRigidBody([this](btRigidBody *rb) {
      auto motion_state = rb->getMotionState();

      btTransform transform;
      motion_state->getWorldTransform(transform);

      vec3 origin = transform.getOrigin();

      printf("object(0x%p) at { %.2f, %.2f, %.2f }\n", rb,
        origin.x, origin.y, origin.z);
    });
  }
}

void DynamicsWorld::foreachObject(CollisionObjectIter fn)
{
  auto& objects = m_world->getCollisionObjectArray();
  for(auto i = objects.size()-1; i >= 0; i--) {
    auto obj = objects[i];
    fn(obj);
  }
}

void DynamicsWorld::foreachRigidBody(RigidBodyIter fn)
{
  foreachObject([&](btCollisionObject *obj) {
    if(auto rb = btRigidBody::upcast(obj)) {
      fn(rb);
    }
  });
}

}