#pragma once

#include <common.h>

#include <math/geometry.h>

class btVector3;

class btTransform;

class btDynamicsWorld;

class btMotionState;
class btCollisionShape;
class btCollisionObject;
class btRigidBody;

namespace bt {

class CollisionShapeManager;

void init();
void finalize();

CollisionShapeManager& shapes();

btVector3 to_btVector3(const vec3& v);
vec3 from_btVector3(const btVector3& v);

}