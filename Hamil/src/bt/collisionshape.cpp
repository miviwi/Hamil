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
  auto e = to_btVector3(half_extents);
  return initShape(new btBoxShape(e));
}

CollisionShape CollisionShapeManager::sphere(float radius)
{
  return initShape(new btSphereShape(radius));
}

CollisionShape CollisionShapeManager::convexHull(float *verts, size_t num_verts, size_t stride)
{
  return initShape(new btConvexHullShape((btScalar *)verts, (int)num_verts, (int)stride));
}

CollisionShape CollisionShapeManager::convexHull(std::function<bool(vec3& dst)> next_vert)
{
  auto self = new btConvexHullShape();

  // Add all the vertices one-by-one
  vec3 vert;
  while(next_vert(vert)) self->addPoint(to_btVector3(vert));

  return initShape(self);
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

CollisionShape CollisionShapeManager::initShape(CollisionShape&& shape)
{
  shape.m->setUserPointer(this);
  m_shapes.insert(shape);

  return std::move(shape);
}

}