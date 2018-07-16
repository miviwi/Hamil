#pragma once

#include <python/python.h>

#include <string>
#include <utility>
#include <type_traits>

namespace python {

class TypeObject;

class List;
class Dict;
class Tuple;

// RAII wrapper around PyObject (does Py_DECREF automatically)
//   - nullptr can be passed to the constructor as the 'object'
//     and will be handled correctly, though no checking is done
//     when attempting to run methods on such an Object
//   - NEVER put Object's or any other derived types in global scope
//     they are initialized before the interpreter and will cause 
//     undefined behaviour.
//     To overcome this limitation do:
//       Object obj = nullptr;
//       void init_obj()
//       {
//         obj = ...; // init 'obj' however you like
//       }
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

  bool deref();

  PyObject *move();

  PyObject *py() const;
  PyObject *operator *() const;

  operator bool() const;

  template <typename T>
  bool typeCheck() const
  {
    static_assert(std::is_base_of_v<Object, T>, "invalid T passed to Object::typeCheck()!");
    return py() ? T::py_type_check(py()) : false;
  }

  template <typename T> T as() const& { return typeCheck<T>() ? ref(py()) : nullptr; }
  template <typename T> T as() &&     { return typeCheck<T>() ? move() : nullptr; }

  std::string str() const;
  std::string repr() const;

  Object attr(const Object& name) const;
  Object attr(const char *name) const;
  void attr(const Object& name, const Object& value);
  void attr(const char *name, const Object& value);

  bool callable() const;

  Object call(Tuple args, Dict kwds);
  Object call(Tuple args);
  Object call();

  template <typename... Args>
  Object call(Args&&... args)
  {
    auto tuple = PyTuple_Pack(sizeof...(args), args.py()...);
    return doCall(tuple); // cleans up 'tuple'
  }
  template <typename... Args>
  Object operator()(Args&&... args)
  {
    return call(std::forward<Args>(args)...);
  }

  List dir() const;

private:
  Object doCall(PyObject *args);

  PyObject *m;
};

}