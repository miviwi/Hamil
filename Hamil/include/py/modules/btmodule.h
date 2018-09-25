#pragma once

#include <py/python.h>

namespace bt {
class RigidBody;
}

namespace py {

PyObject *PyInit_bt();

int RigidBody_Check(PyObject *obj);
PyObject *RigidBody_FromRigidBody(bt::RigidBody rb);

}