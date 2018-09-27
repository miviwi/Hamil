#pragma once

#include <py/python.h>

namespace bt {
class RigidBody;
class CollisionShape;
}

namespace py {

PyObject *PyInit_bt();

int RigidBody_Check(PyObject *obj);
PyObject *RigidBody_FromRigidBody(bt::RigidBody rb);

int CollisionShapeCheck(PyObject *obj);
PyObject *CollisionShape_FromCollisionShape(bt::CollisionShape shape);

}