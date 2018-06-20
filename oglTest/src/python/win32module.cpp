#include <python/win32module.h>
#include <python/module.h>
#include <python/object.h>
#include <python/types.h>
#include <python/collections.h>

#include <Windows.h>

#include <win32/time.h>

namespace python {

struct Win32ModuleToken;
static MethodDefList<Win32ModuleToken> Win32ModuleMethods;

struct TimeToken;
static MemberDefList<TimeToken> TimeMembers;
static MethodDefList<TimeToken> TimeMethods;

struct Time {
  PyObject_HEAD;

  win32::Time m;
};

static int Time_Check(PyObject *obj);
static PyObject *Time_FromTime(win32::Time t);

#define TIME_INIT_ERR "Time can only be initailized with another Time object"

static int Time_Init(Time *self, PyObject *args, PyObject *kwds)
{
  if(PyTuple_Size(args) > 1 || kwds) {
    PyErr_SetString(PyExc_ValueError, TIME_INIT_ERR);
    return -1;
  }

  if(PyTuple_Size(args) == 1) {
    PyObject *t = PyTuple_GET_ITEM(args, 0);
    if(Time_Check(t)) {
      self->m = ((Time *)t)->m;
    } else if(PyLong_Check(t)) {
      self->m = PyLong_AsUnsignedLongLong(t);
    } else {
      PyErr_SetString(PyExc_TypeError, TIME_INIT_ERR);
      return -1;
    }
  } else {
    self->m = win32::InvalidTime;
  }

  return 0;
}

static PyObject *Time_Repr(Time *self)
{
  return Unicode::from_format("Time(%llu)", self->m).move();
}

static PyObject *Time_Str(Time *self)
{
  return Unicode::from_format("%llu", self->m).move();
}

static PyObject *Time_Ticks(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return Time_FromTime(win32::Timers::ticks());
}

static PyObject *Time_Time(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return PyFloat_FromDouble(win32::Timers::timef_s());
}

static TypeObject Time_Type =
  TypeObject()
    .name("Time")
    .doc("opaque value representing time")
    .size(sizeof(Time))
    .init((initproc)Time_Init)
    .methods(TimeMethods(
      MethodDef()
        .name("ticks")
        .doc("gives a Time object which represents an opaque time value mesured from system start")
        .method(Time_Ticks)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("time")
        .doc("gives the time in seconds since start")
        .method(Time_Time)
        .flags(METH_CLASS | METH_NOARGS)))
    .repr((reprfunc)Time_Repr)
    .str((reprfunc)Time_Str)
  ;

static int Time_Check(PyObject *object)
{
  return Time_Type.check(object);
}

static PyObject *Time_FromTime(win32::Time t)
{
  return (PyObject *)Time_Type.newObject<Time>();
}

struct TimersToken;
static MemberDefList<TimersToken> TimersMembers;
static MethodDefList<TimersToken> TimersMethods;


static ModuleDef Win32Module =
  ModuleDef()
    .name("Win32")
    .doc("interface to various win32 apis")
  ;

PyObject *PyInit_win32()
{
  auto self = Module::create(Win32Module.py())
    .addType("Time", Time_Type)
    ;

  return *self;
}

}