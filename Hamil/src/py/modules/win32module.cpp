#include <py/modules/win32module.h>
#include <py/module.h>
#include <py/object.h>
#include <py/types.h>
#include <py/collections.h>

#include <os/time.h>

#include <type_traits>

namespace py {

struct TimeToken;

[[maybe_unused]]
static MemberDefList<TimeToken> TimeMembers;
static MethodDefList<TimeToken> TimeMethods;
static GetSetDefList<TimeToken> TimeGetSet;

struct Time {
  PyObject_HEAD;

  os::Time m;
};

static int Time_Check(PyObject *obj);
static PyObject *Time_FromTime(os::Time t);

#define TIME_INIT_ERR "Time can only be initailized with another Time object or a number (of seconds)"

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
    } else if(PyNumber_Check(t)) {
      Float f = PyNumber_Float(t);
      self->m = os::Timers::s_to_ticks(f.f());
    } else {
      PyErr_SetString(PyExc_TypeError, TIME_INIT_ERR);
      return -1;
    }
  } else {
    self->m = os::InvalidTime;
  }

  return 0;
}

static PyObject *Time_Repr(Time *self)
{
  return Unicode::from_format("Time(%lf)", os::Timers::ticks_to_sf(self->m)).move();
}

static PyObject *Time_Str(Time *self)
{
  return Unicode::from_format("%lf", os::Timers::ticks_to_sf(self->m)).move();
}

static PyObject *Time_Time(PyObject *Py_UNUSED(klass), PyObject *Py_UNUSED(arg))
{
  return PyFloat_FromDouble(os::Timers::ticks());
}

static PyObject *Time_To_S(Time *self, void *Py_UNUSED(closure))
{
  return PyFloat_FromDouble(os::Timers::ticks_to_sf(self->m));
}

static PyObject *Time_To_Ms(Time *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLongLong(os::Timers::ticks_to_ms(self->m));
}

static PyObject *Time_To_Us(Time *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLongLong(os::Timers::ticks_to_us(self->m));
}

static TypeObject Time_Type =
  TypeObject()
    .name("win32.Time")
    .doc("opaque value representing time")
    .size(sizeof(Time))
    .init((initproc)Time_Init)
    .methods(TimeMethods(
      MethodDef()
        .name("ticks")
        .doc("returns a Time object which is incremented monotonically since from system start")
        .method(Time_Time)
        .flags(METH_CLASS | METH_NOARGS)))
    .getset(TimeGetSet(
      GetSetDef()
        .name("s")
        .doc("converts the Time object to seconds")
        .get((getter)Time_To_S),
      GetSetDef()
        .name("ms")
        .doc("converts the Time object to milliseconds")
        .get((getter)Time_To_Ms),
      GetSetDef()
        .name("us")
        .doc("converts the Time object to microseconds")
        .get((getter)Time_To_Us)))
    .repr((reprfunc)Time_Repr)
    .str((reprfunc)Time_Str)
  ;

static int Time_Check(PyObject *object)
{
  return Time_Type.check(object);
}

static PyObject *Time_FromTime(os::Time t)
{
  auto obj = Time_Type.newObject<Time>();
  obj->m = t;

  return (PyObject *)obj;
}

struct TimerToken;

[[maybe_unused]]
static MemberDefList<TimerToken> TimerMembers;
static MethodDefList<TimerToken> TimerMethods;

struct Timer {
  PyObject_HEAD;

  os::Timer m;
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
    .name("win32.Timer")
    .doc("Base type for various <...>Timer objects.\nHas no real utility on it's own")
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
  return Timer_Type.newObject();
}

struct DeltaTimerToken;

[[maybe_unused]]
static MemberDefList<DeltaTimerToken> DeltaTimerMembers;
[[maybe_unused]]
static MethodDefList<DeltaTimerToken> DeltaTimerMethods;
static GetSetDefList<DeltaTimerToken> DeltaTimerGetSet;

struct DeltaTimer {
  PyObject_HEAD;

  os::DeltaTimer m;
};

static int DeltaTimer_Check(PyObject *obj);
static PyObject *DeltaTimer_New();

static PyObject *DeltaTimer_ElapsedTicks(DeltaTimer *self, void *Py_UNUSED(closure))
{
  return Time_FromTime(self->m.elapsedTicks());
}

static TypeObject DeltaTimer_Type =
  TypeObject()
    .name("win32.DeltaTimer")
    .doc("gives the amount of time elapsed since reset()")
    .size(sizeof(DeltaTimer))
    .base(Timer_Type)
    .getset(DeltaTimerGetSet(
      GetSetDef()
        .name("delta")
        .doc("returns a Time object representing time passed since reset()")
        .get((getter)DeltaTimer_ElapsedTicks)))
  ;

static int DeltaTimer_Check(PyObject *obj)
{
  return DeltaTimer_Type.check(obj);
}

static PyObject *DeltaTimer_New()
{
  return DeltaTimer_Type.newObject();
}

struct DurationTimerToken;

[[maybe_unused]]
static MemberDefList<DurationTimerToken> DurationTimerMembers;
[[maybe_unused]]
static MethodDefList<DurationTimerToken> DurationTimerMethods;
static GetSetDefList<DurationTimerToken> DurationTimerGetSet;

struct DurationTimer {
  PyObject_HEAD;

  os::DurationTimer m;
};

static int DurationTimer_Check(PyObject *obj);
static PyObject *DurationTimer_New();

static PyObject *DurationTimer_GetDuration(DurationTimer *self, void *Py_UNUSED(closure))
{
  return Time_FromTime(self->m.duration());
}

static int DurationTimer_SetDuration(DurationTimer *self, PyObject *value, void *Py_UNUSED(closure))
{
  if(PyNumber_Check(value)) {
    Float f = PyNumber_Float(value);

    self->m.durationSeconds(f.f());
    return 0;
  } else if(Time_Check(value)) {
    self->m.durationTicks(((Time *)value)->m);
    return 0;
  }

  PyErr_SetString(PyExc_TypeError, "DurationTimer.duration can only be set to a Time object or a number");
  return -1;
}

static PyObject *DurationTimer_Elapsed(DurationTimer *self, void *Py_UNUSED(closure))
{
  return PyFloat_FromDouble(self->m.elapsedf());
}

static PyObject *DurationTimer_HasElapsed(DurationTimer *self, void *Py_UNUSED(closure))
{
  return PyBool_FromLong(self->m.elapsed());
}

static TypeObject DurationTimer_Type =
  TypeObject()
    .name("win32.DurationTimer")
    .doc("tracks how much of DurationTimer.duration has passed the since reset()")
    .size(sizeof(DurationTimer))
    .base(Timer_Type)
    .getset(DurationTimerGetSet(
      GetSetDef()
        .name("duration")
        .doc("get/set the duration via a Time object or a number")
        .get((getter)DurationTimer_GetDuration)
        .set((setter)DurationTimer_SetDuration),
      GetSetDef()
        .name("elapsed")
        .doc("returns a value in the range <0; 1> representing how much of the duration has passed since reset()")
        .get((getter)DurationTimer_Elapsed),
      GetSetDef()
        .name("has_elapsed")
        .doc("returns true when DurationTimer.elapsed >= 1.0")
        .get((getter)DurationTimer_HasElapsed)))
  ;

static int DurationTimer_Check(PyObject *obj)
{
  return DurationTimer_Type.check(obj);
}

static PyObject *DurationTimer_New()
{
  return DurationTimer_Type.newObject();
}

struct LoopTimerToken;

[[maybe_unused]]
static MemberDefList<LoopTimerToken> LoopTimerMembers;
[[maybe_unused]]
static MethodDefList<LoopTimerToken> LoopTimerMethods;
[[maybe_unused]]
static GetSetDefList<LoopTimerToken> LoopTimerGetSet;

struct LoopTimer {
  PyObject_HEAD;

  os::LoopTimer m;
};

static int LoopTimer_Check(PyObject *obj);
static PyObject *LoopTimer_New();

static PyObject *LoopTimer_GetDuration(LoopTimer *self, void *Py_UNUSED(closure))
{
  return Time_FromTime(self->m.duration());
}

static int LoopTimer_SetDuration(LoopTimer *self, PyObject *value, void *Py_UNUSED(closure))
{
  if(PyNumber_Check(value)) {
    Float f = PyNumber_Float(value);

    self->m.durationSeconds(f.f());
    return 0;
  } else if(Time_Check(value)) {
    self->m.durationTicks(((Time *)value)->m);
    return 0;
  }

  PyErr_SetString(PyExc_TypeError, "LoopTimer.duration can only be set to a Time object or a number");
  return -1;
}

static PyObject *LoopTimer_Elapsed(LoopTimer *self, void *Py_UNUSED(closure))
{
  return PyFloat_FromDouble(self->m.elapsedf());
}

static PyObject *LoopTimer_HasElapsed(LoopTimer *self, void *Py_UNUSED(closure))
{
  return PyBool_FromLong(self->m.elapsed());
}

static PyObject *LoopTimer_ElapsedLoops(LoopTimer *self, void *Py_UNUSED(closure))
{
  return PyFloat_FromDouble(self->m.elapsedLoopsf());
}

static TypeObject LoopTimer_Type = 
  TypeObject()
    .name("win32.LoopTimer")
    .doc("tracks duration passed analogously to DurationTimer, extending that by also counting the number of loops")
    .size(sizeof(LoopTimer))
    .base(Timer_Type)
    .getset(DurationTimerGetSet(
      GetSetDef()
        .name("duration")
        .doc("get/set the duration via a Time object or a number")
        .get((getter)LoopTimer_GetDuration)
        .set((setter)LoopTimer_SetDuration),
      GetSetDef()
        .name("elapsed")
        .doc("returns a value in the range <0; 1> representing how much of the duration has passed since reset()")
        .get((getter)LoopTimer_Elapsed),
      GetSetDef()
        .name("has_elapsed")
        .doc("returns true when LoopTimer.elapsed >= 1.0")
        .get((getter)LoopTimer_HasElapsed),
      GetSetDef()
        .name("elapsed_loops")
        .doc("loops since reset() in the integer part and progress in the current loop in the fraction part")
        .get((getter)LoopTimer_ElapsedLoops)))
  ;

static int LoopTimer_Check(PyObject *obj)
{
  return LoopTimer_Type.check(obj);
}

static PyObject *LoopTimer_New()
{
  return LoopTimer_Type.newObject();
}

struct Win32ModuleToken;

[[maybe_unused]]
static MethodDefList<Win32ModuleToken> Win32ModuleMethods;

static ModuleDef Win32Module =
  ModuleDef()
    .name("win32")
    .doc("interface to various win32 (system) apis")
  ;

PyObject *PyInit_win32()
{
  auto self = Module::create(Win32Module.py())
    .addType(Time_Type)

    .addType(Timer_Type)
    .addType(DeltaTimer_Type)
    .addType(DurationTimer_Type)
    .addType(LoopTimer_Type)
    ;

  return *self;
}

}
