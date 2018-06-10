#include <python/mathmodule.h>
#include <python/module.h>
#include <python/types.h>
#include <python/collections.h>

#include <math/geometry.h>

#include <string>

namespace python {

static void ArgTypeError(const char *message)
{
  PyErr_SetString(PyExc_TypeError, message);
}

struct MathToken;
static MethodDefList<MathToken> MathMethods;

struct Vec2Token;
static MemberDefList<Vec2Token> Vec2Members;
static MethodDefList<Vec2Token> Vec2Methods;

struct vec2 {
  PyObject_HEAD;

  ::vec2 m;
};

static int Vec2_Check(PyObject *obj);
static PyObject *Vec2_FromVec2(::vec2 v);

#define VEC2_INIT_ERR "argument to vec2() must be number(s) or another vec2"

static int Vec2_Init(vec2 *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);
  static char *kwds_names[] = { "x", "y", nullptr };

  if(num_args == 1) {
    if(!Vec2_Check(PyTuple_GET_ITEM(args, 0))) {
      PyErr_SetString(PyExc_TypeError, VEC2_INIT_ERR);
      return -1;
    }
    auto other = (vec2 *)PyTuple_GET_ITEM(args, 0);

    self->m = other->m;
    return 1;
  } else {
    float x = 0, y = 0;
    if(!PyArg_ParseTupleAndKeywords(args, kwds,
                                    "|ff;" VEC2_INIT_ERR, kwds_names,
                                    &x, &y))
      return -1;

    self->m = { x, y };
    return 0;
  }

  return -1;
}

static PyObject *Vec2_Repr(vec2 *self)
{
  return Unicode::from_format("vec2(%.2f, %.2f)", self->m.x, self->m.y).move();
}

static PyObject *Vec2_Str(vec2 *self)
{
  return Unicode::from_format("[%.2f, %.2f]", self->m.x, self->m.y).move();
}

static PyObject *Vec2_Add(vec2 *self, PyObject *other)
{
  if(!Vec2_Check(other)) {
    ArgTypeError("argument to '+' must be a vec2");
    return nullptr;
  }

  auto b = (vec2 *)other;
  return Vec2_FromVec2(self->m + b->m);
}

static PyObject *Vec2_Sub(vec2 *self, PyObject *other)
{
  if(!Vec2_Check(other)) {
    ArgTypeError("argument to '-' must be a vec2");
    return nullptr;
  }

  auto b = (vec2 *)other;
  return Vec2_FromVec2(self->m - b->m);
}

static PyObject *Vec2_Mul(vec2 *self, PyObject *other)
{
  if(Vec2_Check(other)) {
    auto b = (vec2 *)other;
    return Vec2_FromVec2(self->m * b->m);
  } else if(PyNumber_Check(other)) {
    Float u = PyNumber_Float(other);
    return Vec2_FromVec2(self->m * (float)u.f());
  }

  ArgTypeError("argument to '*' must be a vec2 or number");
  return nullptr;
}

static PyObject *Vec2_Length(vec2 *self, PyObject *Py_UNUSED(arg))
{
  return PyFloat_FromDouble(self->m.length());
}

static PyObject *Vec2_Dot(vec2 *self, PyObject *arg)
{
  if(Vec2_Check(arg)) {
    auto b = (vec2 *)arg;
    return PyFloat_FromDouble(self->m.dot(b->m));
  }

  ArgTypeError("argument to dot() must be a vec2");
  return nullptr;
}

static PyObject *Vec2_Normalize(vec2 *self, PyObject *Py_UNUSED(arg))
{
  return Vec2_FromVec2(self->m.normalize());
}

static PyObject *Vec2_Distance2(vec2 *self, PyObject *arg)
{
  if(Vec2_Check(arg)) {
    auto b = (vec2 *)arg;
    return PyFloat_FromDouble(self->m.distance2(b->m));
  }

  ArgTypeError("argument to distance2() must be a vec2");
  return nullptr;
}

static PyObject *Vec2_Distance(vec2 *self, PyObject *arg)
{
  if(Vec2_Check(arg)) {
    auto b = (vec2 *)arg;
    return PyFloat_FromDouble(self->m.distance(b->m));
  }

  ArgTypeError("argument to distance() must be a vec2");
  return nullptr;
}

static PyNumberMethods Vec2NumberMethods = {
  (binaryfunc) Vec2_Add, /* nb_add */
  (binaryfunc) Vec2_Sub, /* nb_subtract */
  (binaryfunc) Vec2_Mul, /* nb_multiply */
  nullptr, /* nb_remainder */
  nullptr, /* nb_divmod */
  nullptr, /* nb_power */
  nullptr, /* nb_negative */
  nullptr, /* nb_positive */
  nullptr, /* nb_absolute */
  nullptr, /* nb_bool */
  nullptr, /* nb_invert */
  nullptr, /* nb_lshift */
  nullptr, /* nb_rshift */
  nullptr, /* nb_and */
  nullptr, /* nb_xor */
  nullptr, /* nb_or */
  nullptr, /* nb_int */
  nullptr, /* nb_reserved */
  nullptr, /* nb_float */
  
  nullptr, /* nb_inplace_add */
  nullptr, /* nb_inplace_subtract */
  nullptr, /* nb_inplace_multiply */
  nullptr, /* nb_inplace_remainder */
  nullptr, /* nb_inplace_power */
  nullptr, /* nb_inplace_lshift */
  nullptr, /* nb_inplace_rshift */
  nullptr, /* nb_inplace_and */
  nullptr, /* nb_inplace_xor */
  nullptr, /* nb_inplace_or */
  
  nullptr, /* nb_floor_divide */
  nullptr, /* nb_true_divide */
  nullptr, /* nb_inplace_floor_divide */
  nullptr, /* nb_inplace_true_divide */
  
  nullptr, /* nb_index */
  
  nullptr, /* nb_matrix_multiply */
  nullptr, /* nb_inplace_matrix_multiply */
};

static TypeObject Vec2_Type =
  TypeObject()
    .name("math.vec2")
    .doc("2D vector")
    .size(sizeof(vec2))
    .init((initproc)Vec2_Init)
    .members(Vec2Members(
      MemberDef()
        .name("x")
        .offset(offsetof(vec2, m.x))
        .type(T_FLOAT),
      MemberDef()
        .name("y")
        .offset(offsetof(vec2, m.y))
        .type(T_FLOAT)))
    .methods(Vec2Methods(
      MethodDef()
        .name("length")
        .method(Vec2_Length)
        .flags(METH_NOARGS),
      MethodDef()
        .name("dot")
        .method(Vec2_Dot)
        .flags(METH_O),
      MethodDef()
        .name("distance2")
        .method(Vec2_Distance2)
        .flags(METH_O),
      MethodDef()
        .name("distance")
        .method(Vec2_Distance)
        .flags(METH_O),
      MethodDef()
        .name("normalize")
        .doc("convert to unit vector")
        .method(Vec2_Normalize)
        .flags(METH_NOARGS)))
    .number_methods(&Vec2NumberMethods)
    .repr((reprfunc)Vec2_Repr)
    .str((reprfunc)Vec2_Str)
  ;

int Vec2_Check(PyObject *obj)
{
  return Vec2_Type.check(obj);
}

PyObject *Vec2_FromVec2(::vec2 v)
{
  auto obj = Vec2_Type.newObject<vec2>();
  obj->m = v;

  return (PyObject *)obj;
}

struct Vec3Token;
static MemberDefList<Vec3Token> Vec3Members;
static MethodDefList<Vec3Token> Vec3Methods;

struct vec3 {
  PyObject_HEAD;
  
  ::vec3 m;
};

static int Vec3_Check(PyObject *obj);
static PyObject *Vec3_FromVec3(::vec3 v);

#define VEC3_INIT_ERR "argument to vec3() must be one of number(s), vec3"

static int Vec3_Init(vec3 *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(num_args == 1) {
    if(Vec3_Check(PyTuple_GET_ITEM(args, 0))) {
      auto other = (vec3 *)PyTuple_GET_ITEM(args, 0);

      self->m = other->m;
      return 1;
    }

    PyErr_SetString(PyExc_TypeError, VEC3_INIT_ERR);
    return -1;
  } else {
    float x = 0, y = 0, z = 0;
    if(!PyArg_ParseTupleAndKeywords(args, kwds,
                                    "|fff;" VEC3_INIT_ERR, kwds_names,
                                    &x, &y, &z))
      return -1;

    self->m = { x, y, z };
    return 0;
  }

  return -1;
}

static PyObject *Vec3_Repr(vec3 *self)
{
  return Unicode::from_format("vec3(%.2f, %.2f, %.2f)", self->m.x, self->m.y, self->m.z).move();
}

static PyObject *Vec3_Str(vec3 *self)
{
  return Unicode::from_format("[%.2f, %.2f, %.2f]", self->m.x, self->m.y, self->m.z).move();
}

static PyObject *Vec3_Add(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to '+' must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return Vec3_FromVec3(self->m + b->m);
}

static PyObject *Vec3_Sub(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to '-' must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return Vec3_FromVec3(self->m - b->m);
}

static PyObject *Vec3_Mul(vec3 *self, PyObject *other)
{
  if(Vec3_Check(other)) {
    auto b = (vec3 *)other;
    return Vec3_FromVec3(self->m * b->m);
  } else if(PyNumber_Check(other)) {
    Float u = PyNumber_Float(other);
    return Vec3_FromVec3(self->m * (float)u.f());
  }

  ArgTypeError("argument to '*' must be a vec3 or number");
  return nullptr;
}

static PyObject *Vec3_XY(vec3 *self, PyObject *Py_UNUSED(arg))
{
  return Vec2_FromVec2(self->m.xy());
}

static PyObject *Vec3_Length(vec3 *self, PyObject *Py_UNUSED(arg))
{
  return PyFloat_FromDouble(self->m.length());
}

static PyObject *Vec3_Cross(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to cross() must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return Vec3_FromVec3(self->m.cross(b->m));
}

static PyNumberMethods Vec3NumberMethods = {
  (binaryfunc)Vec3_Add, /* nb_add */
  (binaryfunc)Vec3_Sub, /* nb_subtract */
  (binaryfunc)Vec3_Mul, /* nb_multiply */
  nullptr, /* nb_remainder */
  nullptr, /* nb_divmod */
  nullptr, /* nb_power */
  nullptr, /* nb_negative */
  nullptr, /* nb_positive */
  nullptr, /* nb_absolute */
  nullptr, /* nb_bool */
  nullptr, /* nb_invert */
  nullptr, /* nb_lshift */
  nullptr, /* nb_rshift */
  nullptr, /* nb_and */
  nullptr, /* nb_xor */
  nullptr, /* nb_or */
  nullptr, /* nb_int */
  nullptr, /* nb_reserved */
  nullptr, /* nb_float */

  nullptr, /* nb_inplace_add */
  nullptr, /* nb_inplace_subtract */
  nullptr, /* nb_inplace_multiply */
  nullptr, /* nb_inplace_remainder */
  nullptr, /* nb_inplace_power */
  nullptr, /* nb_inplace_lshift */
  nullptr, /* nb_inplace_rshift */
  nullptr, /* nb_inplace_and */
  nullptr, /* nb_inplace_xor */
  nullptr, /* nb_inplace_or */

  nullptr, /* nb_floor_divide */
  nullptr, /* nb_true_divide */
  nullptr, /* nb_inplace_floor_divide */
  nullptr, /* nb_inplace_true_divide */

  nullptr, /* nb_index */

  nullptr, /* nb_matrix_multiply */
  nullptr, /* nb_inplace_matrix_multiply */
};

static TypeObject Vec3_Type = 
  TypeObject()
    .name("vec3")
    .doc("3D Vector")
    .size(sizeof(vec3))
    .init((initproc)Vec3_Init)
    .members(Vec3Members(
      MemberDef()
        .name("x")
        .offset(offsetof(vec3, m.x))
        .type(T_FLOAT),
      MemberDef()
        .name("y")
        .offset(offsetof(vec3, m.y))
        .type(T_FLOAT),
      MemberDef()
        .name("z")
        .offset(offsetof(vec3, m.z))
        .type(T_FLOAT)))
    .methods(Vec3Methods(
      MethodDef()
        .name("xy")
        .method(Vec3_XY)
        .flags(METH_NOARGS),
      MethodDef()
        .name("length")
        .method(Vec3_Length)
        .flags(METH_NOARGS),
      MethodDef()
        .name("cross")
        .method(Vec3_Cross)
        .flags(METH_O)))
    .number_methods(&Vec3NumberMethods)
    .repr((reprfunc)Vec3_Repr)
    .str((reprfunc)Vec3_Str)
  ;

static int Vec3_Check(PyObject *obj)
{
  return Vec3_Type.check(obj);
}

static PyObject *Vec3_FromVec3(::vec3 v)
{
  auto obj = Vec3_Type.newObject<vec3>();
  obj->m = v;

  return (PyObject *)obj;
}

struct Vec4Token;
static MemberDefList<Vec4Token> Vec4Members;
static MethodDefList<Vec4Token> Vec4Methods;

struct vec4 {
  PyObject_HEAD;

  ::vec4 m;
};

static int Vec4_Check(PyObject *obj);
static PyObject *Vec4_FromVec4(::vec4 v);

#define VEC4_INIT_ERR "argument to vec4() must be one of number(s), vec2, vec3, vec4"

static int Vec4_Init(vec4 *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);
  static char *kwds_names[] = { "x", "y", "z", "w", nullptr };

  if(num_args == 1) {
    auto other = PyTuple_GET_ITEM(args, 0);
    if(Vec4_Check(other)) {
      self->m = ((vec4 *)other)->m;
    } else if(Vec3_Check(other)) {
      self->m = ((vec3 *)other)->m;
    } else if(Vec2_Check(other)) {
      self->m = ((vec2 *)other)->m;
    } else {
      PyErr_SetString(PyExc_TypeError, VEC4_INIT_ERR);
      return -1;
    }

    return 1;
  } else {
    float x = 0, y = 0, z = 0, w = 1;
    if(!PyArg_ParseTupleAndKeywords(args, kwds,
                                    "|ffff;" VEC4_INIT_ERR, kwds_names,
                                    &x, &y, &z, &w))
      return -1;

    self->m = { x, y, z, w };
    return 0;
  }

  return -1;
}

static PyObject *Vec4_Repr(vec4 *self)
{
  return Unicode::from_format("vec4(%.2f, %.2f, %.2f, %.2f)",
                              self->m.x, self->m.y, self->m.z, self->m.w).move();
}

static PyObject *Vec4_Str(vec4 *self)
{
  return Unicode::from_format("[%.2f, %.2f, %.2f, %.2f]", self->m.x, self->m.y, self->m.z, self->m.w).move();
}

static PyObject *Vec4_Add(vec4 *self, PyObject *other)
{
  if(!Vec2_Check(other)) {
    ArgTypeError("argument to '+' must be a vec4");
    return nullptr;
  }

  auto b = (vec4 *)other;
  return Vec4_FromVec4(self->m + b->m);
}

static PyObject *Vec4_Sub(vec4 *self, PyObject *other)
{
  if(!Vec4_Check(other)) {
    ArgTypeError("argument to '-' must be a vec4");
    return nullptr;
  }

  auto b = (vec4 *)other;
  return Vec4_FromVec4(self->m - b->m);
}

static PyObject *Vec4_Mul(vec4 *self, PyObject *other)
{
  if(Vec4_Check(other)) {
    auto b = (vec4 *)other;
    return Vec4_FromVec4(self->m * b->m);
  } else if(PyNumber_Check(other)) {
    Float u = PyNumber_Float(other);
    return Vec4_FromVec4(self->m * (float)u.f());
  }

  ArgTypeError("argument to '*' must be a vec4 or number");
  return nullptr;
}

static PyNumberMethods Vec4NumberMethods = {
  (binaryfunc)Vec4_Add, /* nb_add */
  (binaryfunc)Vec4_Sub, /* nb_subtract */
  (binaryfunc)Vec4_Mul, /* nb_multiply */
  nullptr, /* nb_remainder */
  nullptr, /* nb_divmod */
  nullptr, /* nb_power */
  nullptr, /* nb_negative */
  nullptr, /* nb_positive */
  nullptr, /* nb_absolute */
  nullptr, /* nb_bool */
  nullptr, /* nb_invert */
  nullptr, /* nb_lshift */
  nullptr, /* nb_rshift */
  nullptr, /* nb_and */
  nullptr, /* nb_xor */
  nullptr, /* nb_or */
  nullptr, /* nb_int */
  nullptr, /* nb_reserved */
  nullptr, /* nb_float */

  nullptr, /* nb_inplace_add */
  nullptr, /* nb_inplace_subtract */
  nullptr, /* nb_inplace_multiply */
  nullptr, /* nb_inplace_remainder */
  nullptr, /* nb_inplace_power */
  nullptr, /* nb_inplace_lshift */
  nullptr, /* nb_inplace_rshift */
  nullptr, /* nb_inplace_and */
  nullptr, /* nb_inplace_xor */
  nullptr, /* nb_inplace_or */

  nullptr, /* nb_floor_divide */
  nullptr, /* nb_true_divide */
  nullptr, /* nb_inplace_floor_divide */
  nullptr, /* nb_inplace_true_divide */

  nullptr, /* nb_index */

  nullptr, /* nb_matrix_multiply */
  nullptr, /* nb_inplace_matrix_multiply */
};

static TypeObject Vec4_Type = 
  TypeObject()
    .name("vec4")
    .doc("4D vector")
    .size(sizeof(vec4))
    .init((initproc)Vec4_Init)
    .members(Vec4Members(
      MemberDef()
        .name("x")
        .offset(offsetof(vec4, m.x))
        .type(T_FLOAT),
      MemberDef()
        .name("y")
        .offset(offsetof(vec4, m.y))
        .type(T_FLOAT),
      MemberDef()
        .name("z")
        .offset(offsetof(vec4, m.z))
        .type(T_FLOAT),
      MemberDef()
        .name("w")
        .offset(offsetof(vec4, m.w))
        .type(T_FLOAT)))
    .number_methods(&Vec4NumberMethods)
    .repr((reprfunc)Vec4_Repr)
    .str((reprfunc)Vec4_Str)
  ;

int Vec4_Check(PyObject *obj)
{
  return Vec4_Type.check(obj);
}

static PyObject *Vec4_FromVec4(::vec4 v)
{
  auto obj = Vec4_Type.newObject<vec4>();
  obj->m = v;

  return (PyObject *)obj;
}

static ModuleDef MathModule = 
  ModuleDef()
    .name("Math")
    .doc("Vector and Matrix types and various 3D graphics related operations.")
  ;

PyObject *PyInit_math()
{
  auto self = Module::create(MathModule.py())
    .addType("vec2", Vec2_Type)
    .addType("vec3", Vec3_Type)
    .addType("vec4", Vec4_Type)
    .addObject("pi", Float(PI))
    ;
        
  return *self;
}

}