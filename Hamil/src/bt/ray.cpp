#include <bt/ray.h>
#include <bt/rigidbody.h>
#include <bt/btcommon.h>

namespace bt {

Ray Ray::from_direction(vec3 origin, vec3 direction)
{
  Ray r;

  r.m_from = origin;
  r.m_to   = direction.normalize() * 10e10; // Extend the ray to "infinity"

  return r;
}

Ray Ray::from_to(vec3 from, vec3 to)
{
  Ray r;

  r.m_from = from;
  r.m_to   = to;

  return r;
}

RayClosestHit::RayClosestHit() :
  m_collision_object(nullptr)
{
}

RigidBody RayClosestHit::rigidBody() const
{
  if(auto rb = btRigidBody::upcast(m_collision_object)) {
    return RigidBody((btRigidBody *)rb);
  }

  return RigidBody();
}

vec3 RayClosestHit::point() const
{
  return m_hit_point;
}

vec3 RayClosestHit::normal() const
{
  return m_hit_normal;
}

RayClosestHit::operator bool() const
{
  return m_collision_object;
}

}