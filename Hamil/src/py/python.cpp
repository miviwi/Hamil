#include <py/python.h>
#include <py/object.h>
#include <py/types.h>
#include <py/collections.h>
#include <py/module.h>
#include <py/exception.h>

#include <py/modules/win32module.h>
#include <py/modules/mathmodule.h>
#include <py/modules/yamlmodule.h>
#include <py/modules/resmodule.h>
#include <py/modules/btmodule.h>
#include <py/modules/hmmodule.h>

#include <marshal.h>

#include <vector>

namespace py {

Dict p_globals(nullptr);

static ModuleDef hpy = 
  ModuleDef()
    .name("hpy")
  ;

PyObject *PyInit_hpy()
{
  auto self = Module::create(hpy.py())
    .addObject("win32", PyInit_win32())
    .addObject("math",  PyInit_math())
    .addObject("yaml",  PyInit_yaml())
    .addObject("res",   PyInit_res())
    .addObject("bt",    PyInit_bt())
    //.addObject("hm",    PyInit_hm())
    ;

  return *self;
}

static _inittab p_modules[] ={
  { "hpy", PyInit_hpy },

  { nullptr }
};

void init()
{
  PyImport_ExtendInittab(p_modules);

  Py_InitializeEx(0);
  p_globals = Dict();

  if(auto mod_builtin = Module::import("builtins")) {
    set_global("__builtins__", mod_builtin);
  } else {
    throw Exception::fetch();
  }
}

void finalize()
{
  p_globals = nullptr; // Py_DECREF's the underlying PyObject

  Py_Finalize();
}

int main(int argc, char *argv[])
{
  wchar_t **wargv = new wchar_t*[argc];
  for(int i = 0; i < argc; i++) {
    wargv[i] = Py_DecodeLocale(argv[i], nullptr);
  }

  PyImport_ExtendInittab(p_modules);

  auto exit_code = Py_Main(argc, wargv);

  for(int i = 0; i < argc; i++) {
    PyMem_RawFree(wargv[i]);
  }
  delete[] wargv;

  return exit_code;
}

Object p_compile_string(const char *src, int start)
{
  Object co = Py_CompileString(src, "$", start);
  if(!co) throw Exception::fetch();

  return co;
}

void exec(const char *input)
{
  if(*input == '\0') return;

  PyEval_EvalCode(*p_compile_string(input, Py_single_input), *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();
}

Object eval(const char *input)
{
  if(*input == '\0') return None();

  Object result = PyEval_EvalCode(*p_compile_string(input, Py_eval_input), *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();

  return result;
}

void run_script(const char *input)
{
  if(*input == '\0') return;

  PyEval_EvalCode(*p_compile_string(input, Py_file_input), *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();
}

Object load(const void *code, size_t sz)
{
  return PyMarshal_ReadObjectFromString((const char *)code, (ssize_t)sz);
}

Object load(const std::vector<char>& code)
{
  return load(code.data(), code.size());
}

Object deserialize(const void *code, size_t sz)
{
  auto co = load(code, sz);

  Object result = PyEval_EvalCode(*co, *p_globals, nullptr);
  if(Exception::occured()) throw Exception::fetch();

  return result;
}

Object deserialize(const std::vector<char>& code)
{
  return deserialize(code.data(), code.size());
}

static std::vector<char> p_compile_helper(const char *src, const char *filename, int mode)
{
  Object co = Py_CompileString(src, filename, mode);
  if(!co) throw Exception::fetch();

  Bytes code_bytes = PyMarshal_WriteObjectToString(*co, Py_MARSHAL_VERSION);
  if(!code_bytes) throw Exception::fetch();

  std::vector<char> code(code_bytes.size());
  memcpy(code.data(), code_bytes.data(), code.size());

  return code;
}

std::vector<char> compile(const char *src, const char *filename)
{
  return p_compile_helper(src, filename, Py_file_input);
}

std::vector<char> serialize(const char *src, const char *filename)
{
  return p_compile_helper(src, filename, Py_eval_input);
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
