#pragma once

#include <common.h>

#include <math/geometry.h>

class btVector3;

class btTransform;

class btDynamicsWorld;
class btCollisionConfiguration;
class btDispatcher;
class btBroadphaseInterface;
class btConstraintSolver;

class btMotionState;

class btCollisionObject;
class btRigidBody;

namespace bt {

void init();
void finalize();

btVector3 to_btVector3(vec3& v);

}