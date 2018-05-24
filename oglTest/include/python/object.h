#pragma once

#include <python/python.h>

#include <string>

namespace python {

// RAII wrapper around PyObject (does Py_DECREF automatically)
//   - nullptr can be passed to the constructor as the 'object'
//     and will be handled correctly
class Object {
public:
  Object(PyObject *object);
  Object(const Object& other);
  ~Object();

  Object& operator=(const Object& other);
  // Steals reference
  Object& operator=(PyObject *object);

  operator bool() const;

  PyObject *get() const;
  PyObject *operator *() const;

  std::string repr() const;

private:
  PyObject *m;
};


}