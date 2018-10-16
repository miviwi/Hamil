#include <bt/collisionshape.h>
#include <bt/btcommon.h>

#include <cassert>

namespace bt {

CollisionShape::CollisionShape() :
  m(nullptr)
{
}

CollisionShape::CollisionShape(btCollisionShape *shape) :
  m(shape)
{
}

btCollisionShape *CollisionShape::bt() const
{
  return m;
}

void *CollisionShape::get() const
{
  return bt();
}

const char *CollisionShape::name() const
{
  return m->getName();
}

CollisionShapeManager& CollisionShape::manager() const
{
  return *(CollisionShapeManager *)m->getUserPointer();
}

vec3 CollisionShape::calculateLocalInertia(float mass)
{
  btVector3 inertia;
  m->calculateLocalInertia(mass, inertia);

  return from_btVector3(inertia);
}

CollisionShape::operator bool() const
{
  return get();
}

bool CollisionShape::operator==(const CollisionShape& other) const
{
  return get() == other.get();
}

bool CollisionShape::operator!=(const CollisionShape& other) const
{
  return get() != other.get();
}

CollisionShapeManager::CollisionShapeManager()
{
  m_shapes.reserve(InitialAlloc);
}

CollisionShapeManager::~CollisionShapeManager()
{
  for(auto& shape : m_shapes) {
    delete shape.m;
  }
}

CollisionShape CollisionShapeManager::box(vec3 half_extents)
{
  CollisionShape self = new btBoxShape(to_btVector3(half_extents));
  self.m->setUserPointer(this);

  m_shapes.insert(self);

  return self;
}

CollisionShape CollisionShapeManager::sphere(float radius)
{
  CollisionShape self = new btSphereShape(radius);
  self.m->setUserPointer(this);

  m_shapes.insert(self);

  return self;
}

void CollisionShapeManager::destroy(CollisionShape& shape)
{
  // Compare the ref-count to 2 because one reference is always held by 'm_shapes'
  assert(shape.refs() == 2 && "Attempted to destroy a CollisionShape with more than 1 ref!");

  delete shape.m;
  shape.m = nullptr;
  shape.deref();

  m_shapes.erase(shape); // The final deref() will occur here automatically
}

}