#include <bt/bullet.h>
#include <bt/collisionshape.h>
#include <bt/btcommon.h>

#include <memory>

namespace bt {

std::unique_ptr<CollisionShapeManager> p_shapes;

void init()
{
  p_shapes.reset(new CollisionShapeManager());
}

void finalize()
{
  p_shapes.reset();
}

CollisionShapeManager& shapes()
{
  return *p_shapes;
}

btVector3 to_btVector3(const vec3& v)
{
  return { v.x, v.y, v.z };
}

vec3 from_btVector3(const btVector3& v)
{
  return vec3((const float *)v);
}

}