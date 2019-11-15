#pragma once

#include <bt/bullet.h>
#include <bt/rigidbody.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <functional>

namespace os {
// Forward declaration
class ReaderWriterLock;
}

namespace bt {

class Ray;
class RayClosestHit;

// PIMPL classes

class DynamicsWorldConfig;
class DynamicsWorldData;

// - <add,remove>RigidBody(), rayTestClosest() and step()
//   are all thread-safe with relation to each other,
//   HOWEVER the actual RigidBodies themselves are NOT
//   so care must be taken to not read-from or write-to
//   ones in use
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

  // If this method is called from a secondary thread
  //   during simulation adding the body will be deferred
  //   until it ends
  void addRigidBody(RigidBody rb);
  // Same as addRigidBody() (see note above)
  void removeRigidBody(RigidBody rb);

  RayClosestHit rayTestClosest(Ray ray);

  void step(float dt);

private:
  using BtCollisionObjectIter = std::function<void(btCollisionObject *)>;
  using BtRigidBodyIter       = std::function<void(btRigidBody *)>;

  os::ReaderWriterLock& lock();

  // Iterates BACKWARDS
  void foreachObject(BtCollisionObjectIter fn);

  // Iterates BACKWARDS
  void foreachRigidBody(BtRigidBodyIter fn);

  DynamicsWorldData *m_data;

  DynamicsWorldConfig *m_config;
  btDynamicsWorld *m_world;
};

}
