#pragma once

#include <bt/bullet.h>

#include <util/ref.h>

#include <functional>

namespace bt {

class DynamicsWorld : public Ref {
public:
  static constexpr float SimulationRate = 60.0f /* Hz */;
  static constexpr float SimulationStep = 1.0f / SimulationRate;

  static constexpr int SimulationMaxSubsteps = 10;

  using CollisionObjectIter = std::function<void(btCollisionObject *)>;
  using RigidBodyIter       = std::function<void(btRigidBody *)>;

  DynamicsWorld();
  ~DynamicsWorld();

  void initDbgSimulation();
  void startDbgSimulation();
  void stepDbgSimulation(RigidBodyIter fn);

private:

  // Iterates BACKWARDS
  void foreachObject(CollisionObjectIter fn);

  // Iterates BACKWARDS
  void foreachRigidBody(RigidBodyIter fn);

  btCollisionConfiguration *m_collision_config;
  btDispatcher             *m_collision_dispatch;
  btBroadphaseInterface    *m_collision_broadphase;
  btConstraintSolver       *m_collision_solver;

  btDynamicsWorld *m_world;
};

}