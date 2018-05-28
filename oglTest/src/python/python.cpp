#include <python/python.h>
#include <python/object.h>
#include <python/types.h>
#include <python/collections.h>
#include <python/module.h>
#include <python/exception.h>

#include <python/messagebox.h>

#include <marshal.h>

#include <vector>

#include <win32/file.h>

namespace python {

Dict p_globals(nullptr);

static _inittab p_modules[] = {
  { "win32", PyInit_win32 },
  { nullptr }
};

void init()
{
  //PyImport_AppendInittab("win32", python::PyInit_win32);
  PyImport_ExtendInittab(p_modules);

  Py_InitializeEx(0);
  p_globals = Dict();

  if(auto mod_builtin = Module::import("builtins")) {
    set_global("__builtins__", mod_builtin);
  } else {
    throw Exception::fetch();
  }

  win32::File f("test", win32::File::ReadWrite);
  std::vector<char> code(f.size());
  f.read(code.data());

  Object co = load(code);
  set_global("test", Module::exec("test", co));
}

void finalize()
{
  p_globals = nullptr; // Py_DECREF's the underlying PyObject

  Py_Finalize();
}

void exec(const char *input)
{
  Object co = Py_CompileString(input, "$", Py_single_input);
  if(!co) throw Exception::fetch();

  PyEval_EvalCode(*co, *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();
}

Object eval(const char *input)
{
  Object co = Py_CompileString(input, "$", Py_eval_input);
  if(!co) throw Exception::fetch();

  Object result = PyEval_EvalCode(*co, *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();

  return result;
}

Object load(const void *code, size_t sz)
{
  return PyMarshal_ReadObjectFromString((const char *)code, (ssize_t)sz);
}

Object load(const std::vector<char>& code)
{
  return load(code.data(), code.size());
}

std::vector<char> compile(const char *src, const char *filename)
{
  Object co = Py_CompileString(src, filename, Py_file_input); 
  if(!co) throw Exception::fetch();
  
  Bytes code_bytes = PyMarshal_WriteObjectToString(*co, Py_MARSHAL_VERSION);
  if(!code_bytes) throw Exception::fetch();

  std::vector<char> code(code_bytes.size());
  memcpy(code.data(), code_bytes.data(), code.size());

  return code;
}

Object get_global(const char *name)
{
  return p_globals.get(name);
}

void set_global(const char *name, const Object& value)
{
  p_globals.set(name, value);
}

}