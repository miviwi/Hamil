#include <python/messagebox.h>
#include <python/module.h>
#include <python/object.h>
#include <python/types.h>
#include <python/collections.h>

#include <Windows.h>

namespace python {

struct Win32ModuleToken;
static MethodDefList<Win32ModuleToken> Win32ModuleMethods;

struct HANDLETypeToken;
static MemberDefList<HANDLETypeToken> HANDLETypeMembers;

struct HANDLE {
  PyObject_HEAD;

  ::HANDLE h;
};

static TypeObject HANDLEType =
  TypeObject()
    .name("win32.HANDLE")
    .doc("HANDLE wrapper")
    .size(sizeof(HANDLE))
    .members(HANDLETypeMembers(
      MemberDef()
        .name("h")
        .offset(offsetof(HANDLE, h))
        .type(T_PYSSIZET)
        .readonly()
    ));

static PyObject *win32_messagebox(PyObject *self, PyObject *args)
{
  const char *title = nullptr;
  const char *text = nullptr;

  if(!PyArg_ParseTuple(args, "ss:messagebox", &title, &text)) return nullptr;

  MessageBoxA(nullptr, text, title, MB_OK);

  Py_RETURN_NONE;
}

static PyMethodDef Win32Methods[] = {
  { "messagebox", win32_messagebox, METH_VARARGS, 
    "Calls MessageBox() with the chosen title and text." },
  { nullptr },
};

static ModuleDef Win32Module =
  ModuleDef()
    .name("win32")
    .methods(Win32ModuleMethods(
      MethodDef()
        .name("messagebox")
        .method(win32_messagebox)
    ));

PyObject *PyInit_win32()
{
  auto self = Module::create(Win32Module.py())
    .addType("HANDLE", HANDLEType)
    ;

  return *self;
}

}