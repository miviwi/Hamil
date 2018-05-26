#include <python/messagebox.h>
#include <python/module.h>
#include <python/object.h>
#include <python/types.h>
#include <python/collections.h>

#include <Windows.h>

namespace python {

struct HANDLE {
  PyObject_HEAD;

  ::HANDLE h;
};

static TypeObject HANDLEType = TypeObject()
  .name("win32.HANDLE")
  .doc("HANDLE wrapper")
  .size(sizeof(HANDLE))
  ;

static PyObject *win32_messagebox(PyObject *self, PyObject *args)
{
  const char *title = nullptr;
  const char *text = nullptr;

  if(!PyArg_ParseTuple(args, "ss", &title, &text)) return nullptr;

  MessageBoxA(nullptr, text, title, MB_OK);

  Py_RETURN_NONE;
}

static PyMethodDef Win32Methods[] = {
  {
    "messagebox", win32_messagebox, METH_VARARGS,
    "Calls MessageBox with the provided text and title"
  },
  { nullptr, nullptr, 0, nullptr },
};

static ModuleDef Win32Module = ModuleDef()
  .name("win32")
  .methods(Win32Methods)
  ;

PyObject *PyInit_win32()
{
  auto self = Module::create(Win32Module.get())
    .addType("HANDLE", HANDLEType)
    ;

  return *self;
}

}