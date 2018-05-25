#pragma once

#include <python/python.h>
#include <python/object.h>

namespace python {

class None : public Object {
public:
  None();
};

class Numeric : public Object {
public:
  using Object::Object;

  virtual long l() const = 0;
  virtual unsigned long ul() const = 0;
  virtual long long ll() const = 0;
  virtual unsigned long long ull() const = 0;
  virtual double f() const = 0;
  virtual size_t sz() const = 0;
  virtual ssize_t ssz() const = 0;
};

class Long : public Numeric {
public:
  Long(PyObject *object);
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
};

class Float : public Numeric {
public:
  Float(PyObject *object);
  Float(double f);

  virtual long l() const;
  virtual unsigned long ul() const;
  virtual long long ll() const;
  virtual unsigned long long ull() const;
  virtual double f() const;
  virtual size_t sz() const;
  virtual ssize_t ssz() const;
};

class Boolean : public Object {
public:
  Boolean(PyObject *object);
  Boolean(bool b);

  bool val() const;
};

class Unicode : public Object {
public:
  Unicode(PyObject *object);
  Unicode(const char *str);
  Unicode(const char *str, ssize_t sz);

  ssize_t size() const;
  std::string str() const;
};

}