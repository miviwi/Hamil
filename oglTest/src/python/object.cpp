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
  return PyObject_GetAttr(m, *name);
}

Object Object::attr(const char *name) const
{
  return PyObject_GetAttrString(m, name);
}

void Object::attr(const Object& name, const Object& value)
{
  PyObject_SetAttr(m, *name, *value);
}

void Object::attr(const char *name, const Object& value)
{
  PyObject_SetAttrString(m, name, *value);
}

None::None() :
  Object(Py_None)
{
  Py_INCREF(Py_None);
}

Long::Long(PyObject *object) :
  Numeric(object)
{
}

Long::Long(long l) :
  Numeric(PyLong_FromLong(l))
{
}

Long Long::from_ul(unsigned long ul)
{
  return PyLong_FromUnsignedLong(ul);
}

Long Long::from_ll(long long ll)
{
  return PyLong_FromLongLong(ll);
}

Long Long::from_ull(unsigned long long ull)
{
  return PyLong_FromUnsignedLongLong(ull);
}

Long Long::from_f(double f)
{
  return PyLong_FromDouble(f);
}

Long Long::from_sz(size_t sz)
{
  return PyLong_FromSize_t(sz);
}

Long Long::from_ssz(ssize_t ssz)
{
  return PyLong_FromSsize_t(ssz);
}

long Long::l() const
{
  return PyLong_AsLong(py());
}

unsigned long Long::ul() const
{
  return PyLong_AsUnsignedLong(py());
}

long long Long::ll() const
{
  return PyLong_AsLongLong(py());
}

unsigned long long Long::ull() const
{
  return PyLong_AsUnsignedLongLong(py());
}

double Long::f() const
{
  return PyLong_AsDouble(py());
}

size_t Long::sz() const
{
  return PyLong_AsSize_t(py());
}

ssize_t Long::ssz() const
{
  return PyLong_AsSsize_t(py());
}

Float::Float(PyObject *object) :
  Numeric(object)
{
}

Float::Float(double f) :
  Numeric(PyFloat_FromDouble(f))
{
}

long Float::l() const
{
  return (long)f();
}

unsigned long Float::ul() const
{
  return (unsigned long)f();
}

long long Float::ll() const
{
  return (long long)f();
}

unsigned long long Float::ull() const
{
  return (unsigned long long)f();
}

double Float::f() const
{
  return PyFloat_AsDouble(py());
}

size_t Float::sz() const
{
  return (size_t)f();
}

ssize_t Float::ssz() const
{
  return (ssize_t)f();
}

Boolean::Boolean(PyObject *object) :
  Object(object)
{
}

Boolean::Boolean(bool b) :
  Object(PyBool_FromLong((long)b))
{
}

bool Boolean::val() const
{
  return py() == Py_True ? true : false;
}

Unicode::Unicode(PyObject *object) :
  Object(object)
{
}

Unicode::Unicode(const char *str) :
  Object(PyUnicode_FromString(str))
{
}

Unicode::Unicode(const char *str, ssize_t sz) :
  Object(PyUnicode_FromStringAndSize(str, sz))
{
}

ssize_t Unicode::size() const
{
  return PyUnicode_GetLength(py());
}

std::string Unicode::str() const
{
  ssize_t sz = 0;
  const char *data = PyUnicode_AsUTF8AndSize(py(), &sz);

  return std::string(data, (size_t)sz);
}

}