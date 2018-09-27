#include <bt/collisionshape.h>
#include <bt/btcommon.h>

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

vec3 CollisionShape::calculateLocalInertia(float mass)
{
  btVector3 inertia;
  m->calculateLocalInertia(mass, inertia);

  return from_btVector3(inertia);
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
  return new btBoxShape(to_btVector3(half_extents));
}

CollisionShape CollisionShapeManager::sphere(float radius)
{
  return new btSphereShape(radius);
}

}