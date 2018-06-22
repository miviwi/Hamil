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
static GetSetDefList<TimeToken> TimeGetSet;

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

static PyObject *Time_Secs(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return Time_FromTime(win32::Timers::time_s());
}

static PyObject *Time_Millisecs(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return Time_FromTime(win32::Timers::time_ms());
}

static PyObject *Time_Microsecs(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return Time_FromTime(win32::Timers::time_us());
}

static PyObject *Time_TicksPerSec(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return PyLong_FromUnsignedLongLong(win32::Timers::ticks_per_s());
}

static PyObject *Time_To_S(Time *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLongLong(win32::Timers::ticks_to_s(self->m));
}

static PyObject *Time_To_Ms(Time *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLongLong(win32::Timers::ticks_to_ms(self->m));
}

static PyObject *Time_To_Us(Time *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLongLong(win32::Timers::ticks_to_us(self->m));
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
        .doc("returns a Time object which represents an opaque value mesured from system start")
        .method(Time_Ticks)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("time")
        .doc("returns a float which represents the time in seconds since system start")
        .method(Time_Time)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("secs")
        .doc("returns a Time object which reresents the number of seconds since system start")
        .method(Time_Secs)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("millisecs")
        .doc("returns a Time object which reresents the number of milliseconds since system start")
        .method(Time_Millisecs)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("microsecs")
        .doc("returns a Time object which reresents the number of microseconds since system start")
        .method(Time_Microsecs)
        .flags(METH_CLASS | METH_NOARGS),
      MethodDef()
        .name("ticks_per_sec")
        .doc("returns a number which represents the number of times Time.ticks() increments per second")
        .method(Time_TicksPerSec)
        .flags(METH_CLASS | METH_NOARGS)))
    .getset(TimeGetSet(
      GetSetDef()
        .name("s")
        .doc("converts the time object to seconds")
        .get((getter)Time_To_S),
      GetSetDef()
        .name("ms")
        .doc("converts the time object to milliseconds")
        .get((getter)Time_To_Ms),
      GetSetDef()
        .name("us")
        .doc("converts the time object to microseconds")
        .get((getter)Time_To_Us)))
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

struct TimerToken;
static MemberDefList<TimerToken> TimerMembers;
static MethodDefList<TimerToken> TimerMethods;

struct Timer {
  PyObject_HEAD;

  win32::Timer m;
};

static int Timer_Check(PyObject *obj);
static PyObject *Timer_New();

static PyObject *Timer_Reset(Timer *self, PyObject *Py_UNUSED(arg))
{
  self->m.reset();

  Py_RETURN_NONE;
}

static PyObject *Timer_Stop(Timer *self, PyObject *Py_UNUSED(arg))
{
  self->m.stop();

  Py_RETURN_NONE;
}

static TypeObject Timer_Type =
  TypeObject()
    .name("Timer")
    .doc("base type for various <...>Timer objects")
    .size(sizeof(Timer))
    .methods(TimerMethods(
      MethodDef()
        .name("reset")
        .doc("resets the Timer")
        .method(Timer_Reset)
        .flags(METH_NOARGS),
      MethodDef()
        .name("stop")
        .doc("stops the Timer")
        .method(Timer_Stop)
        .flags(METH_NOARGS)))
  ;

int Timer_Check(PyObject *obj)
{
  return Time_Type.check(obj);
}

static PyObject *Timer_New()
{
  return (PyObject *)Timer_Type.newObject<Timer>();
}

struct DeltaTimerToken;
static MemberDefList<DeltaTimerToken> DeltaTimerMembers;
static MethodDefList<DeltaTimerToken> DeltaTimerMethods;
static GetSetDefList<DeltaTimerToken> DeltaTimerGetSet;

struct DeltaTimer {
  PyObject_HEAD;

  win32::DeltaTimer m;
};

static int DeltaTimer_Check(PyObject *obj);
static PyObject *DeltaTimer_New();

static PyObject *DeltaTimer_ElapsedSecondsf(DeltaTimer *self, void *Py_UNUSED(closure))
{
  return PyFloat_FromDouble(self->m.elapsedSecondsf());
}

static PyObject *DeltaTimer_ElapsedTicks(DeltaTimer *self, void *Py_UNUSED(closure))
{
  return Time_FromTime(self->m.elapsedTicks());
}

static TypeObject DeltaTimer_Type =
  TypeObject()
    .name("DeltaTimer")
    .doc("gives the amount of time elapsed since reset()")
    .size(sizeof(DeltaTimer))
    .base(Timer_Type)
    .getset(DeltaTimerGetSet(
      GetSetDef()
        .name("delta")
        .doc("returns a Time object representing time passed since reset()")
        .get((getter)DeltaTimer_ElapsedTicks),
      GetSetDef()
        .name("secs")
        .doc("returns the number of elapsed seconds since reset()")
        .get((getter)DeltaTimer_ElapsedSecondsf)))
  ;

static int DeltaTimer_Check(PyObject *obj)
{
  return DeltaTimer_Type.check(obj);
}

static PyObject *DeltaTimer_New()
{
  return (PyObject *)DeltaTimer_Type.newObject<DeltaTimer>();
}

static ModuleDef Win32Module =
  ModuleDef()
    .name("Win32")
    .doc("interface to various win32 apis")
  ;

PyObject *PyInit_win32()
{
  auto self = Module::create(Win32Module.py())
    .addType("Time", Time_Type)

    .addType("Timer", Timer_Type)
    .addType("DeltaTimer", DeltaTimer_Type)
    ;

  return *self;
}

}