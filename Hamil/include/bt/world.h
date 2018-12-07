#pragma once

#include <bt/bullet.h>
#include <bt/rigidbody.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <functional>

namespace win32 {
class ReaderWriterLock;
}

namespace bt {

class Ray;
class RayClosestHit;

// PIMPL classes

class DynamicsWorldConfig;
class DynamicsWorldData;

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

  RayClosestHit rayTestClosest(Ray ray);

  void step(float dt);

private:
  using BtCollisionObjectIter = std::function<void(btCollisionObject *)>;
  using BtRigidBodyIter       = std::function<void(btRigidBody *)>;

  win32::ReaderWriterLock& lock();

  // Iterates BACKWARDS
  void foreachObject(BtCollisionObjectIter fn);

  // Iterates BACKWARDS
  void foreachRigidBody(BtRigidBodyIter fn);

  DynamicsWorldData *m_data;

  DynamicsWorldConfig *m_config;
  btDynamicsWorld *m_world;
};

}