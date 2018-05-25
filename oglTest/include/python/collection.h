#pragma once

#include <python/python.h>
#include <python/object.h>

#include <initializer_list>
#include <utility>

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

class List : public Object {
public:
  List(PyObject *list);
  List(ssize_t sz);
  List(ObjectRefInitList list);

  Object get(ssize_t index) const;
  void set(ssize_t index, Object& item);
  void set(ssize_t index, Object&& item);

  void append(const Object& item);

  ssize_t size() const;
};

class Dict : public Object {
public:
  Dict(PyObject *dict);
  Dict();
  Dict(ObjectPairInitList list);

  Object get(const Object& key) const;
  Object get(const char *key) const;
  void set(const Object& key, const Object& item);
  void set(const char *key, const Object& item);
};

class Tuple : public Object {
public:
  Tuple(PyObject *tuple);
  Tuple(ssize_t sz);
  Tuple(ObjectRefInitList list);

  Object get(ssize_t index) const;
  void set(ssize_t index, Object& item);
  void set(ssize_t index, Object&& item);
};



}
