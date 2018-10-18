#pragma once

#include <bt/bullet.h>

#include <math/geometry.h>

namespace bt {

class DynamicsWorld;
class RigidBody;

class Ray {
public:
  static Ray from_direction(vec3 origin, vec3 direction);
  static Ray from_to(vec3 from, vec3 to);

protected:
  Ray() = default;

private:
  friend DynamicsWorld;

  vec3 m_from, m_to;
};

class RayClosestHit {
public:
  RayClosestHit();

  RigidBody rigidBody() const;

  vec3 point() const;
  vec3 normal() const;

  // 'true' if the Ray has a hit
  operator bool() const;

private:
  friend DynamicsWorld;

  const btCollisionObject *m_collision_object;

  vec3 m_hit_point, m_hit_normal;
};

}