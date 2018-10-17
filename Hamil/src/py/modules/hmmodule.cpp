#include <py/modules/hmmodule.h>
#include <py/modules/btmodule.h>
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

#include <string>
#include <unordered_map>
#include <functional>

namespace py {

struct ComponentToken;

static GetSetDefList<ComponentToken> ComponentGetSet;

// The layout of this struct relies on the fact that all Components
//   MUST start with the PyObject_HEAD followed by a 'hmRef',
//   which internally is just a pointer i.e. a cast from a
//   hmRef<hm::GameObject> to a hmRef<hm::Component> will produce
//   the correct results (downcast the internal pointer)
struct pComponent {
  PyObject_HEAD;

  hmRef<hm::Component> m;
};

static PyObject *Component_Entity(pComponent *self, void *Py_UNUSED(closure))
{
  return Entity_FromEntity(self->m().entity());
}

static PyObject *Component_GameObject(pComponent *self, void *Py_UNUSED(closure))
{
  return GameObject_FromRef(self->m().entity().component<hm::GameObject>());
}

static TypeObject ComponentType = 
  TypeObject()
    .name("hm.Component")
    .doc("base class for all Components")
    .size(0)
    .getset(ComponentGetSet(
      GetSetDef()
        .name("entity")
        .doc("returns the Component's corresponding Entity")
        .get((getter)Component_Entity),
      GetSetDef()
        .name("gameObject")
        .get((getter)Component_GameObject)))
  ;

struct EntityToken;

static MemberDefList<EntityToken> EntityMembers;
static MethodDefList<EntityToken> EntityMethods;
static GetSetDefList<EntityToken> EntityGetSet;

struct Entity {
  PyObject_HEAD;

  hm::Entity m;
};

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

static PyObject *Entity_GameObject(Entity *self, void *Py_UNUSED(closure));
static PyObject *Entity_RigidBody(Entity *self, void *Py_UNUSED(closure));

static PyObject *Entity_Component(Entity *self, PyObject *arg)
{
  if(!PyType_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "component() must be called with a single type object argument");
    return nullptr;
  }

  auto type      = Type::ref(arg);
  auto component = Type::ref(ComponentType.py());

  // The requested Component's type name
  auto component_name = type.name();

  if(!type.isSubtype(component)) {
    auto err = Unicode::from_format("'%s' is not a Component!", component_name.data());

    PyErr_SetObject(PyExc_ValueError, err.move());
    return nullptr;
  }

  static const std::unordered_map<hm::Component::Tag,
    std::function<PyObject *(Entity *self, void *)>> component_map = {
    { hm::GameObject::tag(), Entity_GameObject },
    { hm::RigidBody::tag(),  Entity_RigidBody },
  };

  auto it = component_map.find(hm::tag_from_string(component_name));
  if(it == component_map.end()) {
    Py_RETURN_NONE;
  }

  return it->second(self, nullptr);
}

static PyObject *Entity_Destroy(Entity *self, PyObject *Py_UNUSED(arg))
{
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

static PyObject *Entity_Repr(Entity *self)
{
  if(!self->m) {
    return PyUnicode_FromString("<Entity::Invalid>");
  }

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

  hm::EntityId a = self->m.id(), b = e->m.id();
  bool result = false;
  switch(op) {
  case Py_EQ: result = a == b; break;
  case Py_NE: result = a != b; break;

  default:  // The op was neiter Py_EQ nor Py_NE
    PyErr_SetNone(PyExc_TypeError);
    return nullptr;
  }

  if(!result) Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

static TypeObject EntityType = 
  TypeObject()
    .name("hm.Entity")
    .doc("wrapper around a hm::Entity")
    .size(sizeof(Entity))
    .init((initproc)Entity_Init)
    .getset(EntityGetSet(
      GetSetDef()
        .name("id")
        .doc("raw Entity id (u32)")
        .get((getter)Entity_Id),
      GetSetDef()
        .name("alive")
        .get((getter)Entity_Alive),
      GetSetDef()
        .name("gameObject")
        .doc("returns the Entity's associated hm.GameObject => e.component(hm.GameObject)")
        .get((getter)Entity_GameObject)))
    .methods(EntityMethods(
      MethodDef()
        .name("component")
        .doc("returns the Component of the given type associated with this Entity (or None)")
        .flags(METH_O)
        .method(Entity_Component),
      MethodDef()
        .name("destroy")
        .doc("destroy the Entity")
        .flags(METH_NOARGS)
        .method(Entity_Destroy)))
    .compare((richcmpfunc)Entity_Compare)
    .repr((reprfunc)Entity_Repr)
    .str((reprfunc)Entity_Str)
  ;

int Entity_Check(PyObject *obj)
{
  return EntityType.check(obj);
}

PyObject *Entity_FromEntity(hm::Entity e)
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

struct GameObjectIterator {
  PyObject_HEAD;

  hm::GameObjectIterator m;
};

static int GameObjectIterator_Check(PyObject *obj);
static PyObject *GameObjectIterator_FromIter(hm::GameObjectIterator it);

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

static PyObject *GameObject_GetIter(GameObject *self)
{
  return GameObjectIterator_FromIter(self->m().begin());
}

static PyObject *GameObject_Children(GameObject *self, void *Py_UNUSED(closure))
{
  return GameObject_GetIter(self);
}

static PyObject *GameObject_Repr(GameObject *self)
{
  if(!self->m) return PyObject_Repr(Py_None);

  return Unicode::from_format("<hm::GameObject of Entity(0x%.8x)>", self->m().entity().id()).move();
}

static TypeObject GameObjectType = 
  TypeObject()
    .name("hm.GameObject")
    .doc("wrapper around a hm::GameObject")
    .base(ComponentType)
    .getset(GameObjectGetSet(
      GetSetDef()
        .name("name")
        .get((getter)GameObject_Name),
      GetSetDef()
        .name("parent")
        .get((getter)GameObject_Parent),
      GetSetDef()
        .name("children")
        .doc("returns an iterator object for the GameObject's children")
        .get((getter)GameObject_Children)))
    .size(sizeof(GameObject))
    .init((initproc)GameObject_Init)
    .repr((reprfunc)GameObject_Repr)
    .str((reprfunc)GameObject_Repr)
  ;

int GameObject_Check(PyObject *obj)
{
  return GameObjectType.check(obj);
}

PyObject *GameObject_FromRef(hm::ComponentRef<hm::GameObject> ref)
{
  auto self = GameObjectType.newObject<GameObject>();
  self->m = ref;

  return (PyObject *)self;
}

static PyObject *GameObjectIterator_GetIter(GameObjectIterator *self)
{
  Py_INCREF(self);
  return (PyObject *)self;
}

static PyObject *GameObjectIterator_Next(GameObjectIterator *self)
{
  if(self->m.atEnd()) {
    return nullptr; // No more elements
  }

  auto next = Entity_FromEntity(*self->m);
  self->m++;

  return next;
}

static TypeObject GameObjectIteratorType =
  TypeObject()
    .name("hm.GameObjectIterator")
    .doc("iterator for a given GameObject's children")
    .size(sizeof(GameObjectIterator))
    .iter((getiterfunc)GameObjectIterator_GetIter)
    .iternext((iternextfunc)GameObjectIterator_Next)
  ;

static int GameObjectIterator_Check(PyObject *obj)
{
  return GameObjectIteratorType.check(obj);
}

static PyObject *GameObjectIterator_FromIter(hm::GameObjectIterator it)
{
  auto self = GameObjectIteratorType.newObject<GameObjectIterator>();
  self->m = it;

  return (PyObject *)self;
}

struct HmRigidBodyToken;

static MemberDefList<HmRigidBodyToken> HmRigidBodyMembers;
static GetSetDefList<HmRigidBodyToken> HmRigidBodyGetSet;

struct HmRigidBody {
  PyObject_HEAD;

  hmRef<hm::RigidBody> m;
};

static int HmRigidBody_Check(PyObject *obj);
static PyObject *HmRigidBody_FromRef(hmRef<hm::RigidBody> ref);

static PyObject *HmRigidBody_Rb(HmRigidBody *self, void *Py_UNUSED(closure))
{
  return RigidBody_FromRigidBody(self->m().rb);
}

static PyObject *HmRigidBody_Shape(HmRigidBody *self, void *Py_UNUSED(closure))
{
  return CollisionShape_FromCollisionShape(self->m().shape);
}

static PyObject *HmRigidBody_Repr(GameObject *self)
{
  if(!self->m) return PyObject_Repr(Py_None);

  return Unicode::from_format("<hm::RigidBody of Entity(0x%.8x)>", self->m().entity().id()).move();
}

static TypeObject HmRigidBodyType = 
  TypeObject()
    .name("hm.RigidBody")
    .base(ComponentType)
    .size(sizeof(HmRigidBody))
    .getset(HmRigidBodyGetSet(
      GetSetDef()
        .name("rb")
        .doc("returns the btRigidBody")
        .get((getter)HmRigidBody_Rb),
      GetSetDef()
        .name("shape")
        .doc("returns the btRigidBody's btCollisionShape")
        .get((getter)HmRigidBody_Shape)))
    .repr((reprfunc)HmRigidBody_Repr)
    .str((reprfunc)HmRigidBody_Repr)
  ;

static int HmRigidBody_Check(PyObject *obj)
{
  return HmRigidBodyType.check(obj);
}

static PyObject *HmRigidBody_FromRef(hmRef<hm::RigidBody> ref)
{
  auto self = HmRigidBodyType.newObject<HmRigidBody>();
  self->m = ref;

  return (PyObject *)self;
}

static PyObject *Entity_GameObject(Entity *self, void *Py_UNUSED(closure))
{
  return GameObject_FromRef(self->m.component<hm::GameObject>());
}

static PyObject *Entity_RigidBody(Entity *self, void *Py_UNUSED(closure))
{
  return HmRigidBody_FromRef(self->m.component<hm::RigidBody>());
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
    .addType(ComponentType)

    // Components
    .addType(GameObjectType)
    .addType(HmRigidBodyType)

    // Misc.
    .addType(GameObjectIteratorType)
    ;

  EntityType.dict().set("Invalid", Long(hm::Entity::Invalid));

  return *self;
}

}