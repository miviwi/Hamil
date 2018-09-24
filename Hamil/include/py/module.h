#pragma once

#include <py/python.h>
#include <py/object.h>

#include <initializer_list>
#include <string>

#include <structmember.h>

namespace py {

class Dict;
class TypeObject;

class Module : public Object {
public:
  Module(PyObject *module);

  static Module import(const char *name);
  static Module create(PyModuleDef *module);
  static Module exec(const char *name, const Object& co, const char *filename = nullptr);

  Module& addType(TypeObject& type);
  Module& addObject(const char *name, Object&& o);
  Module& addInt(const char *name, long value);
  Module& addString(const char *name, const char *value);

  void *state();
};

class MethodDef;
class MemberDef;

// Must be made 'static'!
class ModuleDef {
public:
  ModuleDef();

  ModuleDef& name(const char *name);
  ModuleDef& doc(const char *doc);
  ModuleDef& size(ssize_t sz);

  ModuleDef& methods(PyMethodDef *methods);
  ModuleDef& slots(PyModuleDef_Slot *slots);

  PyModuleDef *py();

private:
  PyModuleDef m;
};

class MethodDef {
public:
  MethodDef();

  MethodDef& name(const char *name);
  MethodDef& doc(const char *doc);
  MethodDef& method(void *fn);
  MethodDef& flags(int flags);

  const PyMethodDef& py() const;

private:
  PyMethodDef m;
};

template <typename T, typename Token>
class DefList {
public:
  template <typename... Args>
  T *operator()(Args&... methods)
  {
    static T m[] = {
      methods.py()...,
    { nullptr }
    };

    return m;
  }
};

// Usage:
//   struct MyModuleToken;
//   static MethodDefList<MyModuleToken> MyModuleMethods;
//
//   MyModuleMethods(
//     MethodDef(), MethodDef(), ...
//   )
//
//   struct MyTypeToken;
//   static MemberDefList<MyTypeToken> MyTypeMembers;
//
//   MyTypeMembers(
//     MemberDef(), MemberDef(), ...
//   )
template <typename T>
class MethodDefList : public DefList<PyMethodDef, T> { };

template <typename T>
class MemberDefList : public DefList<PyMemberDef, T> { };

template <typename T>
class GetSetDefList : public DefList<PyGetSetDef, T> { };

class MemberDef {
public:
  MemberDef();

  MemberDef& name(const char *name);
  MemberDef& doc(const char *doc);
  MemberDef& offset(ssize_t off);
  MemberDef& type(int type);
  MemberDef& readonly();

  const PyMemberDef& py() const;

private:
  PyMemberDef m;
};

class GetSetDef {
public:
  GetSetDef();

  GetSetDef& name(const char *name);
  GetSetDef& doc(const char *doc);
  GetSetDef& get(getter fn);
  GetSetDef& set(setter fn);
  GetSetDef& closure(void *closure);

  const PyGetSetDef& py() const;

private:
  PyGetSetDef m;
};

class TypeObject {
public:
  TypeObject();

  TypeObject& name(const char *name);
  TypeObject& doc(const char *doc);
  TypeObject& size(ssize_t sz);
  TypeObject& itemsize(ssize_t item_sz);
  TypeObject& flags(unsigned long flags);
  TypeObject& base(const TypeObject& base);

  TypeObject& dictoffset(ssize_t off);

  TypeObject& iter(getiterfunc fn);
  TypeObject& iternext(iternextfunc fn);

  TypeObject& new_(newfunc fn);
  TypeObject& init(initproc fn);
  TypeObject& destructor(::destructor fn);

  TypeObject& repr(reprfunc fn);
  TypeObject& str(reprfunc fn);

  TypeObject& methods(PyMethodDef *methods);
  TypeObject& members(PyMemberDef *members);
  TypeObject& getset(PyGetSetDef *getset);

  TypeObject& getattr(getattrofunc getattro);
  TypeObject& setattr(setattrofunc setattro);

  TypeObject& number_methods(PyNumberMethods *number);
  TypeObject& sequence_methods(PySequenceMethods *sequence);
  TypeObject& mapping_methods(PyMappingMethods *mapping);

  TypeObject& compare(richcmpfunc cmp);

  Dict dict();

  PyObject *py();

  template <typename T> T *newObject() { return (T *)newObject(); }
  PyObject *newObject();

  static void freeObject(void *object);

  bool check(PyObject *obj);

private:
  PyTypeObject m;
};

}
