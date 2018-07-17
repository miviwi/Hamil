#include <py/exception.h>
#include <py/object.h>

#include <tuple>

namespace py {

Exception Exception::fetch()
{
  Object otype = nullptr,
    oval = nullptr,
    otrace = nullptr;
  PyObject *type = nullptr,
    *val = nullptr,
    *trace = nullptr;

  PyErr_Fetch(&type, &val, &trace);
  std::tie(otype, oval, otrace) = std::tie(type, val, trace);

  return { otype, oval, otrace };
}

bool Exception::occured()
{
  return PyErr_Occurred();
}

const Object& Exception::type() const
{
  return m_type;
}

const Object& Exception::value() const
{
  return m_val;
}

const Object& Exception::traceback() const
{
  return m_trace;
}

Exception::Exception(Object type, Object value, Object traceback) :
  m_type(type), m_val(value), m_trace(traceback)
{
}

}