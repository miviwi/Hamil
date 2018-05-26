#pragma once

#include <python/python.h>
#include <python/object.h>

namespace python {

class TypeObject;

class Module : public Object {
public:
  Module(PyObject *module);

  static Module import(const char *name);
  static Module create(PyModuleDef *module);

  Module& addType(const char *name, TypeObject& type);
  Module& addObject(const char *name, Object&& o);
  Module& addInt(const char *name, long value);
  Module& addString(const char *name, const char *value);

  void *state();
};

class ModuleDef {
public:
  ModuleDef();

  ModuleDef& name(const char *name);
  ModuleDef& doc(const char *doc);
  ModuleDef& size(ssize_t sz);
  ModuleDef& methods(PyMethodDef *methods);
  ModuleDef& slots(PyModuleDef_Slot *slots);

  PyModuleDef *get();

private:
  PyModuleDef m;
};

class TypeObject {
public:
  TypeObject();

  TypeObject& name(const char *name);
  TypeObject& doc(const char *doc);
  TypeObject& size(ssize_t sz);
  TypeObject& itemsize(ssize_t item_sz);
  TypeObject& flags(unsigned long flags);
  TypeObject& base(PyTypeObject *base);

  TypeObject& methods(PyMethodDef *methods);
  TypeObject& members(PyMemberDef *members);

  PyObject *get();

private:
  PyTypeObject m;
};

}
