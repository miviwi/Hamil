#pragma once

#include <python/python.h>
#include <python/object.h>

#include <functional>
#include <string>

namespace python {

class Type : public Object {
public:
  using Object::Object;

  std::string name() const;

  static bool py_type_check(PyObject *self);
};

class None : public Object {
public:
  None();

  static bool py_type_check(PyObject *self);
};

class Number : public Object {
public:
  Number(PyObject *number);
  Number(Object&& number);

  virtual long l() const;
  virtual unsigned long ul() const;
  virtual long long ll() const;
  virtual unsigned long long ull() const;
  virtual double f() const;
  virtual size_t sz() const;
  virtual ssize_t ssz() const;

  static bool py_type_check(PyObject *self);
};

class Long : public Number {
public:
  Long(PyObject *object);
  Long(Object&& object);
  Long(long l);

  static Long from_ul(unsigned long ul);
  static Long from_ll(long long ll);
  static Long from_ull(unsigned long long ull);
  static Long from_f(double f);
  static Long from_sz(size_t sz);
  static Long from_ssz(ssize_t ssz);

  virtual long l() const;
  virtual unsigned long ul() const;
  virtual long long ll() const;
  virtual unsigned long long ull() const;
  virtual double f() const;
  virtual size_t sz() const;
  virtual ssize_t ssz() const;

  static bool py_type_check(PyObject *self);
};

class Float : public Number {
public:
  Float(PyObject *object);
  Float(Object&& object);
  Float(double f);

  virtual long l() const;
  virtual unsigned long ul() const;
  virtual long long ll() const;
  virtual unsigned long long ull() const;
  virtual double f() const;
  virtual size_t sz() const;
  virtual ssize_t ssz() const;

  static bool py_type_check(PyObject *self);
};

class Boolean : public Object {
public:
  Boolean(PyObject *object);
  Boolean(Object&& object);
  Boolean(bool b);

  bool val() const;

  static bool py_type_check(PyObject *self);
};

class Unicode : public Object {
public:
  Unicode(PyObject *object);
  Unicode(Object&& object);
  Unicode(const char *str);
  Unicode(const char *str, ssize_t sz);

  static Unicode from_format(const char *fmt, ...);

  ssize_t size() const;
  std::string str() const;

  static bool py_type_check(PyObject *self);
};

class Bytes : public Object {
public:
  Bytes(PyObject *object);
  Bytes(Object&& object);
  Bytes(const char *str);
  Bytes(const char *str, ssize_t sz);

  ssize_t size() const;
  std::string str() const;
  const char *c_str() const;

  const void *data() const;

  static bool py_type_check(PyObject *self);
};

class Capsule : public Object {
public:
  Capsule(PyObject *capsule);
  Capsule(Object&& object);
  Capsule(void *ptr, const char *name = nullptr);

  void ptr(void *p);
  void *ptr(const char *name) const;
  void *ptr() const;

  void name(const char *name);
  const char *name() const;

  void context(void *ctx);
  void *context() const;

  static bool py_type_check(PyObject *self);
};

}