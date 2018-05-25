#include <python/collections.h>

namespace python {

Dict::Dict(PyObject *dict) :
  Object(dict)
{
}

Dict::Dict() :
  Object(PyDict_New())
{
}

Dict::Dict(ObjectPairInitList list) :
  Dict()
{
  for(auto& pair : list) PyDict_SetItem(py(), *pair.first, *pair.second);
}

Object Dict::get(const Object& key) const
{
  return ref(PyDict_GetItem(py(), *key));
}

Object Dict::get(const char *key) const
{
  return ref(PyDict_GetItemString(py(), key));
}

void Dict::set(const Object& key, const Object& item)
{
  PyDict_SetItem(py(), *key, *item);
}

void Dict::set(const char *key, const Object& item)
{
  PyDict_SetItemString(py(), key, *item);
}

List::List(PyObject *list) :
  Object(list)
{
}

List::List(ssize_t sz) :
  Object(PyList_New(sz))
{
}

List::List(ObjectRefInitList list) :
  List(list.size())
{
  ssize_t idx = 0;
  for(auto& item : list) PyList_SetItem(py(), idx++, *item);
}

Object List::get(ssize_t index) const
{
  return ref(PyList_GetItem(py(), index));
}

void List::set(ssize_t index, Object& item)
{
  PyList_SetItem(py(), index, *item.ref());
}

void List::set(ssize_t index, Object&& item)
{
  PyList_SetItem(py(), index, item.move());
}

void List::append(const Object& item)
{
  PyList_Append(py(), *item);
}

ssize_t List::size() const
{
  return PyList_Size(py());
}

Tuple::Tuple(PyObject *tuple) :
  Object(tuple)
{
}

Tuple::Tuple(ssize_t sz) :
  Object(PyTuple_New(sz))
{
}

Tuple::Tuple(ObjectRefInitList list) :
  Tuple(list.size())
{
  ssize_t idx = 0;
  for(auto& item : list) PyTuple_SetItem(py(), idx++, *item);
}

Object Tuple::get(ssize_t index) const
{
  return ref(PyTuple_GetItem(py(), index));
}

void Tuple::set(ssize_t index, Object& item)
{
  PyTuple_SetItem(py(), index, *item.ref());
}

void Tuple::set(ssize_t index, Object&& item)
{
  PyTuple_SetItem(py(), index, item.move());
}

ObjectRef::ObjectRef(Object&& object) :
  m(object.move())
{
}

PyObject *ObjectRef::operator*() const
{
  return m;
}

}