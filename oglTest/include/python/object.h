#pragma once

#include <python/python.h>

#include <string>

namespace python {

class TypeObject;

class List;

// RAII wrapper around PyObject (does Py_DECREF automatically)
//   - nullptr can be passed to the constructor as the 'object'
//     and will be handled correctly, though no checking is done
//     when attempting to run methods on such an Object
class Object {
public:
  Object(PyObject *object);
  Object(const Object& other);
  Object(Object&& other);
  ~Object();

  Object& operator=(const Object& other);
  // Steals reference
  Object& operator=(PyObject *object);

  static Object ref(PyObject *object);
  Object& ref();

  PyObject *move();

  PyObject *py() const;
  PyObject *operator *() const;

  operator bool() const;

  std::string str() const;
  std::string repr() const;

  Object attr(const Object& name) const;
  Object attr(const char *name) const;
  void attr(const Object& name, const Object& value);
  void attr(const char *name, const Object& value);

  List dir() const;

private:
  PyObject *m;
};

}