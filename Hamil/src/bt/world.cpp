#include <bt/world.h>
#include <bt/collisionshape.h>
#include <bt/ray.h>
#include <bt/btcommon.h>

#include <math/geometry.h>
#include <win32/rwlock.h>

#include <atomic>
#include <array>
#include <utility>

namespace bt {

// Uncomment to disable multi-threaded synchronization
//#define NO_MT_SAFETY

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

class DynamicsWorldData {
public:
  enum {
    AddRemoveQueueDepth = 64,
  };

  enum QueueOp {
    // Add a RigidBody to the DynamicsWorld
    QueueAdd    = 0,
    // Remove a RigidBody from the DynamicsWorld
    QueueRemove = 1,
  };

  using QueueEntry = std::pair<QueueOp, btRigidBody *>;

  win32::ReaderWriterLock lock;

  std::atomic<uint> queue_head = 0;
  std::array<QueueEntry, AddRemoveQueueDepth> queue;

  void queueAppend(QueueEntry e);
  QueueEntry queuePop();
  bool queueEmpty();
};

void DynamicsWorldData::queueAppend(QueueEntry e)
{
  uint head = queue_head.load();
  if(head+1 < queue.size()) {  // The queue still has space
    if(!queue_head.compare_exchange_strong(head, head+1)) {
      // Somebody raced us - retry
      queueAppend(e);
    }

    // 'head' is where we put our request
    queue[head] = e;
    return;
  }

  // No space in the queue - must stall
  lock.acquireExclusive();
  lock.releaseExclusive();

  queueAppend(e); // Retry
}

DynamicsWorldData::QueueEntry DynamicsWorldData::queuePop()
{
  auto old_head = queue_head.fetch_sub(1);
  assert(old_head && "queuePop() called on empty queue!");

  return queue[old_head - 1];
}

bool DynamicsWorldData::queueEmpty()
{
  return queue_head.load() == 0;
}

DynamicsWorld::DynamicsWorld()
{
  m_data = new DynamicsWorldData();

  m_config = DynamicsWorldConfig::create_default();
  m_world  = m_config->discreteWorldFromConfig();

  m_world->setGravity({ 0.0f, -10.0f, 0.0f });
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

  delete m_data;
  delete m_world;
  delete m_config;
}

void *DynamicsWorld::get() const
{
  return m_world;
}

void DynamicsWorld::addRigidBody(RigidBody rb)
{
#if !defined(NO_MT_SAFETY)
  if(lock().tryAcquireExclusive()) {
    // No one else was accessing the world so we can
    //   add the RigidBody right away
    m_world->addRigidBody(rb.m);

    lock().releaseExclusive();
    return;
  }

  // The world is being mutated/inspected - add the
  //   RigidBody to the queue
  m_data->queueAppend({ DynamicsWorldData::QueueAdd, rb.m });
#else
  m_world->addRigidBody(rb.m);
#endif
}

// See addRigidBody() above for notes
void DynamicsWorld::removeRigidBody(RigidBody rb)
{
#if !defined(NO_MT_SAFETY)
  if(lock().tryAcquireExclusive()) {
    m_world->removeRigidBody(rb.m);

    lock().releaseExclusive();
    return;
  }

  m_data->queueAppend({ DynamicsWorldData::QueueRemove, rb.m });
#else
  m_world->removeRigidBody(rb.m);
#endif
}

RayClosestHit DynamicsWorld::rayTestClosest(Ray ray)
{
  auto from = to_btVector3(ray.m_from);
  auto to   = to_btVector3(ray.m_to);

  btCollisionWorld::ClosestRayResultCallback callback(from, to);

#if !defined(NO_MT_SAFETY)
  lock().acquireShared();  // Inspecting the world
#endif
  m_world->rayTest(from, to, callback);
#if !defined(NO_MT_SAFETY)
  lock().releaseShared();
#endif

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
#if !defined(NO_MT_SAFETY)
  // Mutating the world...
  lock().acquireExclusive();

  m_world->stepSimulation(dt, SimulationMaxSubsteps);

  // Perform the queued ops
  while(!m_data->queueEmpty()) {
    auto e = m_data->queuePop();

    switch(e.first) {
    case DynamicsWorldData::QueueAdd:    m_world->addRigidBody(e.second); break;
    case DynamicsWorldData::QueueRemove: m_world->removeRigidBody(e.second); break;
    }
  }

  // ...done
  lock().releaseExclusive();
#else
  m_world->stepSimulation(dt, SimulationMaxSubsteps);
#endif
}

win32::ReaderWriterLock& DynamicsWorld::lock()
{
  return m_data->lock;
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