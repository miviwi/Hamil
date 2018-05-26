#include <python/module.h>

#include <cstring>

namespace python {

Module::Module(PyObject *module) :
  Object(module)
{
  ref();
}

Module Module::import(const char *name)
{
  auto module = PyImport_AddModule(name);
  Py_INCREF(module); // PyImport_AddModule returns a borrowed reference

  return module;
}

Module Module::create(PyModuleDef *module)
{
  return PyModule_Create(module);
}

Module& Module::addType(const char *name, TypeObject& type)
{
  PyModule_AddObject(py(), name, type.get());
  return *this;
}

Module& Module::addObject(const char *name, Object&& o)
{
  PyModule_AddObject(py(), name, o.move());
  return *this;
}

Module& Module::addInt(const char *name, long value)
{
  PyModule_AddIntConstant(py(), name, value);
  return *this;
}

Module& Module::addString(const char *name, const char *value)
{
  PyModule_AddStringConstant(py(), name, value);
  return *this;
}

void *Module::state()
{
  return PyModule_GetState(py());
}

ModuleDef::ModuleDef()
{
  memset(&m, 0, sizeof(PyModuleDef));
  m = {
    PyModuleDef_HEAD_INIT,
    "", "",
    -1,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
  };
}

ModuleDef& ModuleDef::name(const char *name)
{
  m.m_name = name;
  return *this;
}

ModuleDef& ModuleDef::doc(const char *doc)
{
  m.m_doc = doc;
  return *this;
}

ModuleDef& ModuleDef::size(ssize_t sz)
{
  m.m_size = sz;
  return *this;
}

ModuleDef& ModuleDef::methods(PyMethodDef *methods)
{
  m.m_methods = methods;
  return *this;
}

ModuleDef& ModuleDef::slots(PyModuleDef_Slot *slots)
{
  m.m_slots = slots;
  return *this;
}

PyModuleDef *ModuleDef::get()
{
  return &m;
}

TypeObject::TypeObject()
{
  memset(&m, 0, sizeof(PyTypeObject));
  m = {
    PyVarObject_HEAD_INIT(nullptr, 0)
  };

  m.tp_flags = Py_TPFLAGS_DEFAULT;
  m.tp_new = PyType_GenericNew;
  m.tp_alloc = PyType_GenericAlloc;
  m.tp_free = PyObject_Free;
}

TypeObject& TypeObject::name(const char *name)
{
  m.tp_name = name;
  return *this;
}

TypeObject& TypeObject::doc(const char *doc)
{
  m.tp_doc = doc;
  return *this;
}

TypeObject& TypeObject::size(ssize_t sz)
{
  m.tp_basicsize = sz;
  return *this;
}

TypeObject& TypeObject::itemsize(ssize_t item_sz)
{
  m.tp_itemsize = item_sz;
  return *this;
}

TypeObject& TypeObject::flags(unsigned long flags)
{
  m.tp_flags = flags;
  return *this;
}

TypeObject& TypeObject::base(PyTypeObject *base)
{
  m.tp_base = base;
  return *this;
}

TypeObject& TypeObject::methods(PyMethodDef *methods)
{
  m.tp_methods = methods;
  return *this;
}

TypeObject& TypeObject::members(PyMemberDef *members)
{
  m.tp_members = members;
  return *this;
}

PyObject *TypeObject::get()
{
  if(PyType_Ready(&m) < 0) return nullptr;

  Py_INCREF(&m);
  return (PyObject*)&m;
}

#if 0
void TypeObject::generic_free(void *object)
{
  auto type = Py_TYPE(object);
  if(PyType_IS_GC(type)) {
    PyObject_GC_Del(object);
  } else {
    PyObject_Free(object);
  }

  if(PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) Py_DECREF(type);
}
#endif

}