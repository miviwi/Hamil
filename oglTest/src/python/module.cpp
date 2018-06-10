#include <python/module.h>

#include <cstring>
#include <algorithm>

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

Module Module::exec(const char *name, const Object& co, const char *filename)
{
  return PyImport_ExecCodeModule(name, *co);
}

Module& Module::addType(const char *name, TypeObject& type)
{
  PyModule_AddObject(py(), name, type.py());
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

MethodDef::MethodDef()
{
  memset(&m, 0, sizeof(PyMethodDef));
  m = {
    "", nullptr, METH_VARARGS,
    ""
  };
}

MethodDef& MethodDef::name(const char *name)
{
  m.ml_name = name;
  return *this;
}

MethodDef& MethodDef::doc(const char *doc)
{
  m.ml_doc = doc;
  return *this;
}

MethodDef& MethodDef::method(void *fn)
{
  m.ml_meth = (PyCFunction)fn;
  return *this;
}

MethodDef& MethodDef::flags(int flags)
{
  m.ml_flags = flags;
  return *this;
}

const PyMethodDef& MethodDef::py() const
{
  return m;
}

MemberDef::MemberDef()
{
  memset(&m, 0, sizeof(PyMemberDef));
  m = {
    "", 0, 0, 0,
    ""
  };
}

MemberDef& MemberDef::name(const char *name)
{
  m.name = name;
  return *this;
}

MemberDef& MemberDef::doc(const char* doc)
{
  m.doc = doc;
  return *this;
}

MemberDef& MemberDef::offset(ssize_t off)
{
  m.offset = off;
  return *this;
}

MemberDef& MemberDef::type(int type)
{
  m.type = type;
  return *this;
}

MemberDef& MemberDef::readonly()
{
  m.flags = READONLY;
  return *this;
}

const PyMemberDef& MemberDef::py() const
{
  return m;
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

PyModuleDef *ModuleDef::py()
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

TypeObject& TypeObject::new_(newfunc fn)
{
  m.tp_new = fn;
  return *this;
}

TypeObject& TypeObject::init(initproc fn)
{
  m.tp_init = fn;
  return *this;
}

TypeObject& TypeObject::destructor(::destructor fn)
{
  m.tp_dealloc = fn;
  return *this;
}

TypeObject& TypeObject::repr(reprfunc fn)
{
  m.tp_repr = fn;
  return *this;
}

TypeObject& TypeObject::str(reprfunc fn)
{
  m.tp_str = fn;
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

TypeObject& TypeObject::number_methods(PyNumberMethods *number)
{
  m.tp_as_number = number;
  return *this;
}

TypeObject& TypeObject::sequence_methods(PySequenceMethods *sequence)
{
  m.tp_as_sequence = sequence;
  return *this;
}
 
TypeObject& TypeObject::mapping_methods(PyMappingMethods *mapping)
{
  m.tp_as_mapping = mapping;
  return *this;
}

PyObject *TypeObject::py()
{
  if(PyType_Ready(&m) < 0) return nullptr;

  Py_INCREF(&m);
  return (PyObject*)&m;
}

bool TypeObject::check(PyObject *obj)
{
  return PyObject_TypeCheck(obj, &m);
}

PyObject *TypeObject::newObject()
{
  return PyObject_NEW(PyObject, &m);
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