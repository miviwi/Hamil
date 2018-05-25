#include <python/python.h>
#include <python/object.h>
#include <python/collections.h>
#include <python/module.h>

#include <tuple>

namespace python {

Dict p_globals(nullptr);

void init()
{
  Py_InitializeEx(0);

  p_globals = Dict();

  auto mod_builtin = Module("builtins");
  if(!mod_builtin) throw Exception::fetch();

  p_globals.set("__builtins__", mod_builtin);
}

void finalize()
{
  p_globals = nullptr; // Py_DECREF's the underlying PyObject

  Py_Finalize();
}

std::string eval(const char *input)
{
  Object co = Py_CompileString(input, "$", Py_single_input);
  if(!co) throw Exception::fetch();

  Object result = PyEval_EvalCode(*co, *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();

  auto repr = result.repr();

  return repr;
}

Exception Exception::fetch()
{
  Object otype = nullptr,
    oval = nullptr,
    otrace = nullptr;

  {
    PyObject *type, *val, *trace;
    PyErr_Fetch(&type, &val, &trace);

    std::tie(otype, oval, otrace) = std::tie(type, val, trace);
  }

  return {
    otype ? otype.attr("__name__").str() : "???",
    oval ? oval.repr() : "",
    otrace ? otrace.repr() : ""
  };
}

bool Exception::occured()
{
  return PyErr_Occurred();
}

const std::string& Exception::type() const
{
  return m_type;
}

const std::string& Exception::value() const
{
  return m_val;
}

const std::string& Exception::traceback() const
{
  return m_trace;
}

Exception::Exception(std::string&& type, std::string&& value, std::string&& traceback) :
  m_type(type), m_val(value), m_trace(traceback)
{
}

}