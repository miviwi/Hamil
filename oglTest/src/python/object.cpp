#include <python/object.h>
#include <python/types.h>
#include <python/collections.h>

namespace python {

Object::Object(PyObject *object) :
  m(object)
{
}

Object::Object(const Object& other) :
  m(other.m)
{
  Py_XINCREF(m);
}

Object::Object(Object&& other) :
  m(other.m)
{
  other.m = nullptr;
}

Object::~Object()
{
  Py_XDECREF(m);
}

Object& Object::operator=(const Object& other)
{
  Py_XDECREF(m);
  m = other.m;
  Py_XINCREF(m);

  return *this;
}

Object& Object::operator=(PyObject *object)
{
  Py_XDECREF(m);
  m = object;

  return *this;
}

Object Object::ref(PyObject *object)
{
  Py_XINCREF(object);
  return object;
}

Object& Object::ref()
{
  Py_XINCREF(m);

  return *this;
}

PyObject *Object::move()
{
  auto object = m;
  m = nullptr;

  return object;
}

Object::operator bool() const
{
  return m;
}

PyObject *Object::py() const
{
  return m;
}

PyObject *Object::operator*() const
{
  return py();
}

std::string Object::str() const
{
  if(!py()) return "";

  auto o_str = Unicode(PyObject_Str(m));
  if(!o_str) return "";

  return o_str.str();
}

std::string Object::repr() const
{
  if(!py()) return "#<error>";
  
  auto o_repr = Unicode(PyObject_Repr(m));
  if(!o_repr) return "#<error>";

  return o_repr.str();
}

Object Object::attr(const Object& name) const
{
  return PyObject_GetAttr(py(), *name);
}

Object Object::attr(const char *name) const
{
  return PyObject_GetAttrString(py(), name);
}

void Object::attr(const Object& name, const Object& value)
{
  PyObject_SetAttr(py(), *name, *value);
}

void Object::attr(const char *name, const Object& value)
{
  PyObject_SetAttrString(py(), name, *value);
}

List Object::dir() const
{
  return PyObject_Dir(py());
}

}