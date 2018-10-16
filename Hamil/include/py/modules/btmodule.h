#pragma once

#include <py/python.h>

namespace bt {
class RigidBody;
class CollisionShape;
class DynamicsWorld;
}

namespace py {

PyObject *PyInit_bt();

int RigidBody_Check(PyObject *obj);
PyObject *RigidBody_FromRigidBody(bt::RigidBody rb);

int CollisionShape_Check(PyObject *obj);
PyObject *CollisionShape_FromCollisionShape(bt::CollisionShape shape);

int DynamicsWorld_Check(PyObject *obj);
PyObject *DynamicsWorld_FromDynamicsWorld(bt::DynamicsWorld world);

}