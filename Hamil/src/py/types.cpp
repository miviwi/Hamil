#include <py/types.h>

#include <cstdarg>

namespace py {

None::None() :
  Object(Py_None)
{
  Py_INCREF(Py_None);
}

bool None::py_type_check(PyObject *self)
{
  return self == Py_None;
}

Number::Number(PyObject *number) :
  Object(number)
{
}

Number::Number(Object&& number) :
  Number(number.move())
{
}

long Number::l() const
{
  return Long(PyNumber_Long(py())).l();
}

unsigned long Number::ul() const
{
  return Long(PyNumber_Long(py())).ul();
}

long long Number::ll() const
{
  return Long(PyNumber_Long(py())).ll();
}

unsigned long long Number::ull() const
{
  return Long(PyNumber_Long(py())).ull();
}

double Number::f() const
{
  return Float(PyNumber_Float(py())).f();
}

size_t Number::sz() const
{
  return (size_t)PyNumber_AsSsize_t(py(), nullptr);
}

ssize_t Number::ssz() const
{
  return PyNumber_AsSsize_t(py(), nullptr);
}

bool Number::py_type_check(PyObject *self)
{
  return PyNumber_Check(self);
}

Long::Long(PyObject *object) :
  Number(object)
{
}

Long::Long(Object&& object) :
  Long(object.move())
{
}

Long::Long(long l) :
  Number(PyLong_FromLong(l))
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

bool Long::py_type_check(PyObject *self)
{
  return PyLong_Check(self);
}

Float::Float(PyObject *object) :
  Number(object)
{
}

Float::Float(Object&& object) :
  Float(object.move())
{
}

Float::Float(double f) :
  Number(PyFloat_FromDouble(f))
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

bool Float::py_type_check(PyObject *self)
{
  return PyFloat_Check(self);
}

Boolean::Boolean(PyObject *object) :
  Object(object)
{
}

Boolean::Boolean(Object&& object) :
  Boolean(object.move())
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

bool Boolean::py_type_check(PyObject *self)
{
  return PyBool_Check(self);
}

Unicode::Unicode(PyObject *object) :
  Object(object)
{
}

Unicode::Unicode(Object&& object) :
  Unicode(object.move())
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

Unicode::Unicode(const std::string& str) :
  Unicode(str.data(), (ssize_t)str.size())
{
}

static char p_fmt_buf[4096];
Unicode Unicode::from_format(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);

  int len = vsnprintf(p_fmt_buf, sizeof(p_fmt_buf), fmt, va);

  va_end(va);

  return Unicode(p_fmt_buf, len);
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

bool Unicode::py_type_check(PyObject *self)
{
  return PyUnicode_Check(self);
}

Capsule::Capsule(PyObject *capsule) :
  Object(capsule)
{
}

Capsule::Capsule(Object&& object) :
  Capsule(object.move())
{
}

Capsule::Capsule(void *ptr, const char *name) :
  Object(PyCapsule_New(ptr, name, nullptr))
{
}

void Capsule::ptr(void *p)
{
  PyCapsule_SetPointer(py(), p);
}

void *Capsule::ptr(const char *name) const
{
  return PyCapsule_GetPointer(py(), name);
}

void *Capsule::ptr() const
{
  return ptr(name());
}

void Capsule::name(const char *name)
{
  PyCapsule_SetName(py(), name);
}

const char *Capsule::name() const
{
  return PyCapsule_GetName(py());
}

void Capsule::context(void *ctx)
{
  PyCapsule_SetContext(py(), ctx);
}

void *Capsule::context() const
{
  return PyCapsule_GetContext(py());
}

bool Capsule::py_type_check(PyObject *self)
{
  return PyCapsule_CheckExact(self);
}

Bytes::Bytes(PyObject *object) :
  Object(object)
{
}

Bytes::Bytes(Object&& object) :
  Bytes(object.move())
{
}

Bytes::Bytes(const char *str) :
  Object(PyBytes_FromString(str))
{
}

Bytes::Bytes(const char *str, ssize_t sz) :
  Object(PyBytes_FromStringAndSize(str, sz))
{
}

ssize_t Bytes::size() const
{
  return PyBytes_Size(py());
}

std::string Bytes::str() const
{
  return c_str();
}

const char *Bytes::c_str() const
{
  return PyBytes_AsString(py());
}

const void *Bytes::data() const
{
  return c_str();
}

bool Bytes::py_type_check(PyObject *self)
{
  return PyBytes_Check(self);
}

std::string Type::name() const
{
  return attr("__name__").str();
}

bool Type::py_type_check(PyObject *self)
{
  return PyType_Check(self);
}

}