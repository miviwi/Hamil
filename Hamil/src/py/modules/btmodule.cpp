#include <py/modules/hmmodule.h>
#include <py/modules/mathmodule.h>
#include <py/module.h>
#include <py/types.h>
#include <py/collections.h>

#include <math/geometry.h>

#include <bt/bullet.h>
#include <bt/rigidbody.h>

namespace py {

struct RigidBodyToken;

static MemberDefList<RigidBodyToken> RigidBodyMembers;
static MethodDefList<RigidBodyToken> RigidBodyMethods;
static GetSetDefList<RigidBodyToken> RigidBodyGetSet;

struct RigidBody {
  PyObject_HEAD;

  bt::RigidBody m;
};

static int RigidBody_Init(RigidBody *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject *RigidBody_Origin(RigidBody *self, PyObject *Py_UNUSED(closure))
{
  return Vec3_FromVec3(self->m.origin());
}

static PyObject *RigidBody_Activate(RigidBody *self, PyObject *args)
{
  if(PyTuple_Size(args) > 0) {
    PyErr_SetString(PyExc_ValueError, "activate() takes no arguments");
    return nullptr;
  }

  self->m.activate();
  Py_RETURN_NONE;
}

static PyObject *RigidBody_ApplyImpulse(RigidBody *self, PyObject *args)
{
  PyObject *force;
  PyObject *rel_pos;
  if(!PyArg_ParseTuple(args, "OO", &force, &rel_pos)) {
    return nullptr;
  }

  if(!Vec3_Check(force) || !Vec3_Check(rel_pos)) {
    PyErr_SetString(PyExc_TypeError, "applyImpulse() must receive two arguments of type 'vec3'");
    return nullptr;
  }

  self->m.applyImpulse(Vec3_AsVec3(force), Vec3_AsVec3(rel_pos));
  Py_RETURN_NONE;
}

static PyObject *RigidBody_Repr(RigidBody *self)
{
  return Unicode::from_format("<btRigidBody at 0x%p>", self->m.get()).move();
}

static TypeObject RigidBodyType =
  TypeObject()
    .name("RigidBody")
    .doc("btRigidBody wrapper")
    .size(sizeof(RigidBody))
    .init((initproc)RigidBody_Init)
    .getset(RigidBodyGetSet(
      GetSetDef()
        .name("origin")
        .get((getter)RigidBody_Origin)))
    .methods(RigidBodyMethods(
      MethodDef()
        .name("activate")
        .doc("should be called after applyImpulse() etc. to potentially wake up the RigidBody")
        .method(RigidBody_Activate)
        .flags(METH_VARARGS),
      MethodDef()
        .name("applyImpulse")
        .method(RigidBody_ApplyImpulse)
        .flags(METH_VARARGS)))
    .repr((reprfunc)RigidBody_Repr)
    .str((reprfunc)RigidBody_Repr)
  ;

int RigidBody_Check(PyObject *obj)
{
  return RigidBodyType.check(obj);
}

PyObject *RigidBody_FromRigidBody(bt::RigidBody rb)
{
  auto self = RigidBodyType.newObject<RigidBody>();
  self->m = rb;

  return (PyObject *)self;
}

struct BtToken;

static ModuleDef BtModule = 
  ModuleDef()
    .name("bt")
    .doc("Wrapper around Bullet Physics")
  ;

PyObject *PyInit_bt()
{
  auto self = Module::create(BtModule.py())
    .addType(RigidBodyType)
    ;

  return *self;
}

}