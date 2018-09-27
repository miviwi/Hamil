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

void *CollisionShape::get() const
{
  return m;
}

}