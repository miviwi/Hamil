#pragma once

#include <python/python.h>
#include <python/object.h>

#include <initializer_list>
#include <utility>
#include <functional>

namespace python {

class ObjectRef {
public:
  ObjectRef(Object&& object);

  PyObject *operator *() const;

private:
  PyObject *m;
};

using ObjectRefInitList = std::initializer_list<ObjectRef>;

using ObjectPair = std::pair<Object, Object>;
using ObjectPairInitList = std::initializer_list<ObjectPair>;

class List;
class Dict;
class Tuple;

class Collection : public Object {
public:
  using IteratorCallback = std::function<void(Object&)>;

  Collection(PyObject *collection);

  Object get(const Object& key) const;
  void set(const Object& key, const Object& item);

  virtual ssize_t size() const = 0;

  void foreach(IteratorCallback fn);
};

class List : public Collection {
public:
  List(PyObject *list);
  List(Object&& list);
  List(ssize_t sz);
  List(ObjectRefInitList list);

  Object get(ssize_t index) const;
  void set(ssize_t index, Object& item);
  void set(ssize_t index, Object&& item);

  void append(const Object& item);
  void insert(ssize_t where, const Object& item);

  virtual ssize_t size() const;

  static bool py_type_check(PyObject *self);
};

class Dict : public Collection {
public:
  Dict(PyObject *dict);
  Dict(Object&& dict);
  Dict();
  Dict(ObjectPairInitList list);

  Object get(const Object& key) const;
  Object get(const char *key) const;
  void set(const Object& key, const Object& item);
  void set(const char *key, const Object& item);

  virtual ssize_t size() const;

  static bool py_type_check(PyObject *self);
};

class Tuple : public Collection {
public:
  Tuple(PyObject *tuple);
  Tuple(Object&& tuple);
  Tuple(ssize_t sz);
  Tuple(ObjectRefInitList list);

  Object get(ssize_t index) const;
  void set(ssize_t index, Object& item);
  void set(ssize_t index, Object&& item);

  virtual ssize_t size() const;

  static bool py_type_check(PyObject *self);
};

}
