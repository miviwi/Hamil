#pragma once

#include <py/python.h>

namespace hm {
class Entity;
struct GameObject;

template <typename T>
class ComponentRef;
}

namespace py {

PyObject *PyInit_hm();

int Entity_Check(PyObject *obj);
PyObject *Entity_FromEntity(hm::Entity e);

int GameObject_Check(PyObject *obj);
PyObject *GameObject_FromRef(hm::ComponentRef<hm::GameObject> ref);

}