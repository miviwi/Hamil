#pragma once

#include <bt/bullet.h>
#include <bt/rigidbody.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <functional>

namespace bt {

// PIMPL class
class DynamicsWorldConfig;

class DynamicsWorld : public Ref {
public:
  static constexpr float SimulationRate = 60.0f /* Hz */;
  static constexpr float SimulationStep = 1.0f / SimulationRate;

  static constexpr int SimulationMaxSubsteps = 10;

  using RigidBodyIter = std::function<void(const RigidBody&)>;

  DynamicsWorld();
  ~DynamicsWorld();

  // Returns the underlying handle
  //   to a btDynamicsWorld as a void *
  void *get() const;

  void addRigidBody(RigidBody rb);
  void removeRigidBody(RigidBody rb);

  void initDbgSimulation();
  void startDbgSimulation();
  RigidBody createDbgSimulationRigidBody(vec3 sphere, bool active = true);
  void stepDbgSimulation(float dt);
  void stepDbgSimulation(float dt, RigidBodyIter fn);
  RigidBody pickDbgSimulation(vec3 ray_from, vec3 ray_to, vec3& hit_normal);

private:
  using BtCollisionObjectIter = std::function<void(btCollisionObject *)>;
  using BtRigidBodyIter       = std::function<void(btRigidBody *)>;

  // Iterates BACKWARDS
  void foreachObject(BtCollisionObjectIter fn);

  // Iterates BACKWARDS
  void foreachRigidBody(BtRigidBodyIter fn);

  DynamicsWorldConfig *m_config;
  btDynamicsWorld *m_world;
};

}