#include <py/modules/hmmodule.h>
#include <py/modules/mathmodule.h>
#include <py/module.h>
#include <py/types.h>
#include <py/collections.h>

#include <math/geometry.h>

#include <bt/bullet.h>
#include <bt/rigidbody.h>
#include <bt/collisionshape.h>
#include <bt/world.h>

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

static PyObject *RigidBody_Activate(RigidBody *self, PyObject *Py_UNUSED(arg))
{
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
    .name("bt.RigidBody")
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
        .flags(METH_NOARGS),
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

struct CollisionShapeToken;

static MemberDefList<CollisionShapeToken> CollisionShapeMembers;
static MethodDefList<CollisionShapeToken> CollisionShapeMethods;
static GetSetDefList<CollisionShapeToken> CollisionShapeGetSet;

struct CollisionShape {
  PyObject_HEAD;

  bt::CollisionShape m;
};

static int CollisionShape_Init(CollisionShape *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject *CollisionShape_Name(CollisionShape *self, PyObject *Py_UNUSED(closure))
{
  return PyUnicode_FromString(self->m.name());
}

static PyObject *CollisionShape_Repr(CollisionShape *self)
{
  return Unicode::from_format("<btCollisionShape at 0x%p>", self->m.get()).move();
}

static PyObject *CollisionShape_Str(CollisionShape *self)
{
  return CollisionShape_Name(self, nullptr);
}

static TypeObject CollisionShapeType =
  TypeObject()
    .name("bt.CollisionShape")
    .doc("btCollisionShape wrapper")
    .size(sizeof(CollisionShape))
    .getset(CollisionShapeGetSet(
      GetSetDef()
        .name("name")
        .doc("name of the btCollisionShape type (for debugging)")
        .get((getter)CollisionShape_Name)))
    .init((initproc)CollisionShape_Init)
    .repr((reprfunc)CollisionShape_Repr)
    .str((reprfunc)CollisionShape_Str)
  ;

int CollisionShape_Check(PyObject *obj)
{
  return CollisionShapeType.check(obj);
}

PyObject *CollisionShape_FromCollisionShape(bt::CollisionShape shape)
{
  auto self = CollisionShapeType.newObject<CollisionShape>();
  self->m = shape;

  return (PyObject *)self;
}

struct DynamicsWorldToken;

static MemberDefList<DynamicsWorldToken> DynamicsWorldMembers;
static MethodDefList<DynamicsWorldToken> DynamicsWorldMethods;
static GetSetDefList<DynamicsWorldToken> DynamicsWorldGetSet;

struct DynamicsWorld {
  PyObject_HEAD;

  bt::DynamicsWorld m;
};

static int DynamicsWorld_Init(DynamicsWorld *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject *DynamicsWorld_AddRigidBody(DynamicsWorld *self, PyObject *arg)
{
  if(!RigidBody_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "addRigidBody() accepts a single bt.RigidBody argument");
    return nullptr;
  }

  auto rb = (RigidBody *)arg;
  self->m.addRigidBody(rb->m);

  Py_RETURN_NONE;
}

static PyObject *DynamicsWorld_RemoveRigidBody(DynamicsWorld *self, PyObject *arg)
{
  if(!RigidBody_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "removeRigidBody() accepts a single bt.RigidBody argument");
    return nullptr;
  }

  auto rb = (RigidBody *)arg;
  self->m.removeRigidBody(rb->m);

  Py_RETURN_NONE;
}

static PyObject *DynamicsWorld_Repr(DynamicsWorld *self)
{
  return Unicode::from_format("<btDynamicsWorld at 0x%p>", self->m.get()).move();
}

static TypeObject DynamicsWorldType =
  TypeObject()
    .name("bt.DynamicsWorld")
    .doc("btDynamicsWorld wrapper")
    .size(sizeof(DynamicsWorld))
    .methods(DynamicsWorldMethods(
      MethodDef()
        .name("addRigidBody")
        .method(DynamicsWorld_AddRigidBody)
        .flags(METH_O),
      MethodDef()
        .name("removeRigidBody")
        .method(DynamicsWorld_RemoveRigidBody)
        .flags(METH_O)))
    .init((initproc)DynamicsWorld_Init)
    .repr((reprfunc)DynamicsWorld_Repr)
    .str((reprfunc)DynamicsWorld_Repr)
  ;

int DynamicsWorld_Check(PyObject *obj)
{
  return DynamicsWorldType.check(obj);
}

PyObject *DynamicsWorld_FromDynamicsWorld(bt::DynamicsWorld world)
{
  auto self = DynamicsWorldType.newObject<DynamicsWorld>();
  self->m = world;

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
    .addType(CollisionShapeType)
    .addType(DynamicsWorldType)
    ;

  return *self;
}

}