#pragma once

#include <common.h>

class btDynamicsWorld;
class btCollisionConfiguration;
class btDispatcher;
class btBroadphaseInterface;
class btConstraintSolver;

class btCollisionObject;

class btMotionState;

class btRigidBody;

namespace bt {

void init();
void finalize();

}