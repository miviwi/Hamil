#include <py/modules/hmmodule.h>
#include <py/module.h>
#include <py/types.h>
#include <py/collections.h>

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/componentref.h>
#include <hm/components/all.h>

namespace py {

struct EntityToken;
static MemberDefList<EntityToken> EntityMembers;
static MethodDefList<EntityToken> EntityMethods;
static GetSetDefList<EntityToken> EntityGetSet;

struct Entity {
  PyObject_HEAD;

  hm::Entity m;
};

static int Entity_Check(PyObject *obj);
static PyObject *Entity_FromEntity(hm::Entity e);

#define ENTITY_INIT_ERR "Entity can only be initailized with another Entity or an integer id"

static int Entity_Init(Entity *self, PyObject *args, PyObject *kwds)
{
  if(PyTuple_Size(args) > 1 || kwds) {
    PyErr_SetString(PyExc_ValueError, ENTITY_INIT_ERR);
    return -1;
  }

  if(PyTuple_Size(args) == 1) {
    PyObject *e = PyTuple_GET_ITEM(args, 0);
    if(Entity_Check(e)) {
      self->m = ((Entity *)e)->m;
    } else if(PyLong_Check(e)) {
      self->m = (hm::EntityId)PyLong_AsUnsignedLong(e);
    } else {
      PyErr_SetString(PyExc_TypeError, ENTITY_INIT_ERR);
      return -1;
    }
  } else {
    self->m = hm::Entity::Invalid;
  }

  return 0;
}

static PyObject *Entity_Destroy(Entity *self, PyObject *args)
{
  if(PyTuple_Size(args) > 0) {
    PyErr_SetString(PyExc_ValueError, "destroy() must be called with no arguments");
    return nullptr;
  }

  self->m.destroy();
  Py_RETURN_NONE;
}

static PyObject *Entity_Id(Entity *self, void *Py_UNUSED(closure))
{
  return PyLong_FromUnsignedLong(self->m.id());
}

static PyObject *Entity_Alive(Entity *self, void *Py_UNUSED(closure))
{
  return PyBool_FromLong(self->m.alive());
}

static PyObject *Entity_Name(Entity *self, void *Py_UNUSED(closure))
{
  return PyUnicode_FromString(self->m.gameObject().name());
}

static PyObject *Entity_GameObject(Entity *self, void *Py_UNUSED(closure));

static PyObject *Entity_Repr(Entity *self)
{
  return Unicode::from_format("<Entity 0x%.8x>", self->m.id()).move();
}

static PyObject *Entity_Str(Entity *self)
{
  return Unicode::from_format("0x%.8x", self->m.id()).move();
}

static PyObject *Entity_Compare(Entity *self, PyObject *other, int op)
{
  if(!Entity_Check(other)) {
    return Py_NotImplemented;
  }

  auto e = (Entity *)other;
  switch(op) {
  case Py_EQ:
    if(self->m.id() == e->m.id()) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
    break; 

  case Py_NE:
    if(self->m.id() != e->m.id()) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
    break;

  default: PyErr_SetNone(PyExc_TypeError);
  }

  return nullptr;
}

static TypeObject EntityType = 
  TypeObject()
    .name("Entity")
    .doc("wrapper around a hm::Entity")
    .size(sizeof(Entity))
    .init((initproc)Entity_Init)
    .getset(EntityGetSet(
      GetSetDef()
        .name("id")
        .doc("raw Entity id (u32)")
        .get((getter)Entity_Id),
      GetSetDef()
        .name("name")
        .doc("Entity name => gameObject().name()")
        .get((getter)Entity_Name),
      GetSetDef()
        .name("gameObject")
        .get((getter)Entity_GameObject)))
    .methods(EntityMethods(
        MethodDef()
          .name("destroy")
          .doc("destroy the Entity")
          .flags(METH_VARARGS)
          .method(Entity_Destroy)))
    .compare((richcmpfunc)Entity_Compare)
    .repr((reprfunc)Entity_Repr)
    .str((reprfunc)Entity_Str)
  ;

static int Entity_Check(PyObject *obj)
{
  return EntityType.check(obj);
}

static PyObject *Entity_FromEntity(hm::Entity e)
{
  auto obj = EntityType.newObject<Entity>();
  obj->m = e;

  return (PyObject *)obj;
}

struct GameObjectToken;

static MethodDefList<GameObjectToken> GameObjectMethods;
static GetSetDefList<GameObjectToken> GameObjectGetSet;

struct GameObject {
  PyObject_HEAD;

  hmRef<hm::GameObject> m;
};

static int GameObject_Check(PyObject *obj);
static PyObject *GameObject_FromRef(hmRef<hm::GameObject> ref);

static int GameObject_Init(GameObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject *GameObject_Name(GameObject *self, void *Py_UNUSED(closure))
{
  return PyUnicode_FromString(self->m().name());
}

static PyObject *GameObject_Parent(GameObject *self, void *Py_UNUSED(closure))
{
  return Entity_FromEntity(self->m().parent());
}

static TypeObject GameObjectType = 
  TypeObject()
    .name("GameObject")
    .doc("wrapper around a hm::GameObject")
    .getset(GameObjectGetSet(
      GetSetDef()
        .name("name")
        .get((getter)GameObject_Name),
      GetSetDef()
        .name("parent")
        .get((getter)GameObject_Parent)))
    .size(sizeof(GameObject))
    .init((initproc)GameObject_Init)
  ;

static int GameObject_Check(PyObject *obj)
{
  return GameObjectType.check(obj);
}

static PyObject *GameObject_FromRef(hmRef<hm::GameObject> ref)
{
  auto self = GameObjectType.newObject<GameObject>();
  self->m = ref;

  return (PyObject *)self;
}

static PyObject *Entity_GameObject(Entity *self, void *Py_UNUSED(closure))
{
  return GameObject_FromRef(self->m.component<hm::GameObject>());
}

struct HmToken;

static MethodDefList<HmToken> HmMethods;

static PyObject *Hm_FindEntity(PyObject *self, PyObject *arg)
{
  Unicode name = PyObject_Str(arg);

  hm::Entity e = hm::entities().findEntity(name.str());
  return Entity_FromEntity(e);
}

static ModuleDef HmModule = 
  ModuleDef()
    .name("hm")
    .methods(HmMethods(
      MethodDef()
        .name("find_entity")
        .doc("Find hm::Entity by name")
        .method(Hm_FindEntity)
        .flags(METH_O)))
   ;

PyObject *PyInit_hm()
{
  auto self = Module::create(HmModule.py())
    .addType(EntityType)
    .addType(GameObjectType)
    ;

  EntityType.dict().set("Invalid", Long(hm::Entity::Invalid));

  return *self;
}

}