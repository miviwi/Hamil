#include <python/object.h>

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

Object::operator bool() const
{
  return m;
}

PyObject *Object::get() const
{
  return m;
}

PyObject *Object::operator*() const
{
  return get();
}

std::string Object::repr() const
{
  if(!get()) return "#<error>";
  
  Object o_repr = PyObject_Repr(m);
  if(!o_repr) return "#<error>";

  auto ready = PyUnicode_READY(*o_repr);
  if(ready < 0) return "#<error>";

  auto length = PyUnicode_GetLength(*o_repr);
  auto data = (const char *)PyUnicode_1BYTE_DATA(*o_repr);

  return std::string(data, length);
}

}