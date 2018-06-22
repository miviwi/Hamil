#include <python/mathmodule.h>
#include <python/module.h>
#include <python/types.h>
#include <python/collections.h>

#include <math/geometry.h>
#include <math/transform.h>

#include <string>
#include <cstring>

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
    auto other = PyTuple_GET_ITEM(args, 0);
    if(Vec2_Check(other)) {
      self->m = ((vec2 *)other)->m;
    } else {
      ArgTypeError(VEC2_INIT_ERR);
      return -1;
    }

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

  return -1; // unreachable
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
    .doc("2D vector with xy/st components (which alias each other)")
    .size(sizeof(vec2))
    .init((initproc)Vec2_Init)
    .members(Vec2Members(
      MemberDef().name("x").offset(offsetof(vec2, m.x)).type(T_FLOAT),
      MemberDef().name("y").offset(offsetof(vec2, m.y)).type(T_FLOAT),
      MemberDef().name("s").offset(offsetof(vec2, m.s)).type(T_FLOAT),
      MemberDef().name("t").offset(offsetof(vec2, m.t)).type(T_FLOAT)))
    .methods(Vec2Methods(
      MethodDef()
        .name("length")
        .doc("returns the vectors magnitude (length)")
        .method(Vec2_Length)
        .flags(METH_NOARGS),
      MethodDef()
        .name("dot")
        .doc("returns the scalar (dot) product of the vector")
        .method(Vec2_Dot)
        .flags(METH_O),
      MethodDef()
        .name("distance2")
        .doc("returns the squared distance between two vectors")
        .method(Vec2_Distance2)
        .flags(METH_O),
      MethodDef()
        .name("distance")
        .doc("returns the distance between two vectors")
        .method(Vec2_Distance)
        .flags(METH_O),
      MethodDef()
        .name("normalize")
        .doc("returns the vector divided by it's length (which makes it a unit vector/direction)")
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

    ArgTypeError(VEC3_INIT_ERR);
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

  return -1; // unreachable
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

static PyObject *Vec3_Dot(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to dot() must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return PyFloat_FromDouble(self->m.dot(b->m));
}

static PyObject *Vec3_Normalize(vec3 *self, PyObject *Py_UNUSED(arg))
{
  return Vec3_FromVec3(self->m.normalize());
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

static PyObject *Vec3_Distance2(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to distance2() must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return PyFloat_FromDouble(self->m.distance2(b->m));
}

static PyObject *Vec3_Distance(vec3 *self, PyObject *other)
{
  if(!Vec3_Check(other)) {
    ArgTypeError("argument to distance() must be a vec3");
    return nullptr;
  }

  auto b = (vec3 *)other;
  return PyFloat_FromDouble(self->m.distance(b->m));
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
    .doc("3D vector, with xyz/rgb/stp components (which alias each other)")
    .size(sizeof(vec3))
    .init((initproc)Vec3_Init)
    .members(Vec3Members(
      MemberDef().name("x").offset(offsetof(vec3, m.x)).type(T_FLOAT),
      MemberDef().name("y").offset(offsetof(vec3, m.y)).type(T_FLOAT),
      MemberDef().name("z").offset(offsetof(vec3, m.z)).type(T_FLOAT),
      MemberDef().name("r").offset(offsetof(vec3, m.r)).type(T_FLOAT),
      MemberDef().name("g").offset(offsetof(vec3, m.g)).type(T_FLOAT),
      MemberDef().name("b").offset(offsetof(vec3, m.b)).type(T_FLOAT),
      MemberDef().name("s").offset(offsetof(vec3, m.s)).type(T_FLOAT),
      MemberDef().name("t").offset(offsetof(vec3, m.t)).type(T_FLOAT),
      MemberDef().name("p").offset(offsetof(vec3, m.p)).type(T_FLOAT)))
    .methods(Vec3Methods(
      MethodDef()
        .name("xy")
        .doc("returns a vec2 (without the 'z' component)")
        .method(Vec3_XY)
        .flags(METH_NOARGS),
      MethodDef()
        .name("length")
        .doc("returns the mgnitude (length) of the vector")
        .method(Vec3_Length)
        .flags(METH_NOARGS),
      MethodDef()
        .name("dot")
        .doc("returns the scalar (dot) product of the vector")
        .method(Vec3_Dot)
        .flags(METH_O),
      MethodDef()
        .name("normalize")
        .doc("returns the vector divided by it's length (which makes it a unit vector/direction)")
        .method(Vec3_Normalize)
        .flags(METH_NOARGS),
      MethodDef()
        .name("cross")
        .doc("returns the vector (cross) product of the vector")
        .method(Vec3_Cross)
        .flags(METH_O),
      MethodDef()
        .name("distance2")
        .doc("returns the squared distance between two vectors")
        .method(Vec3_Distance2)
        .flags(METH_O),
      MethodDef()
        .name("distance")
        .doc("returns the distance between two vectors")
        .method(Vec3_Distance)
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
      ArgTypeError(VEC4_INIT_ERR);
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

  return -1; // unreachable
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

static PyObject *Vec4_XYZ(vec4 *self, PyObject *Py_UNUSED(arg))
{
  return Vec3_FromVec3(self->m.xyz());
}

static PyObject *Vec4_Length(vec4 *self, PyObject *Py_UNUSED(arg))
{
  return PyFloat_FromDouble(self->m.length());
}

static PyObject *Vec4_Dot(vec4 *self, PyObject *other)
{
  if(!Vec4_Check(other)) {
    ArgTypeError("argument to dot() must be a vec4");
    return nullptr;
  }

  auto b = (vec4 *)other;
  return PyFloat_FromDouble(self->m.dot(b->m));
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
    .doc("4D vector, with xyzw/rgba/stpq components (which alias each other)")
    .size(sizeof(vec4))
    .init((initproc)Vec4_Init)
    .members(Vec4Members(
      MemberDef().name("x").offset(offsetof(vec4, m.x)).type(T_FLOAT),
      MemberDef().name("y").offset(offsetof(vec4, m.y)).type(T_FLOAT),
      MemberDef().name("z").offset(offsetof(vec4, m.z)).type(T_FLOAT),
      MemberDef().name("w").offset(offsetof(vec4, m.w)).type(T_FLOAT),
      MemberDef().name("r").offset(offsetof(vec4, m.r)).type(T_FLOAT),
      MemberDef().name("g").offset(offsetof(vec4, m.g)).type(T_FLOAT),
      MemberDef().name("b").offset(offsetof(vec4, m.b)).type(T_FLOAT),
      MemberDef().name("a").offset(offsetof(vec4, m.a)).type(T_FLOAT),
      MemberDef().name("s").offset(offsetof(vec4, m.s)).type(T_FLOAT),
      MemberDef().name("t").offset(offsetof(vec4, m.t)).type(T_FLOAT),
      MemberDef().name("p").offset(offsetof(vec4, m.p)).type(T_FLOAT),
      MemberDef().name("q").offset(offsetof(vec4, m.q)).type(T_FLOAT)))
    .methods(Vec4Methods(
      MethodDef()
        .name("xyz")
        .doc("returns a vec3 (without the 'w' comopnent)")
        .method(Vec4_XYZ)
        .flags(METH_NOARGS),
      MethodDef()
        .name("length")
         .doc("returns the magnitude (length) of the vector")
        .method(Vec4_Length)
        .flags(METH_NOARGS),
      MethodDef()
        .name("dot")
        .doc("returns the scalar (dot) product of the vector")
        .method(Vec4_Dot)
        .flags(METH_O)))
    .number_methods(&Vec4NumberMethods)
    .repr((reprfunc)Vec4_Repr)
    .str((reprfunc)Vec4_Str)
  ;

static int Vec4_Check(PyObject *obj)
{
  return Vec4_Type.check(obj);
}

static PyObject *Vec4_FromVec4(::vec4 v)
{
  auto obj = Vec4_Type.newObject<vec4>();
  obj->m = v;

  return (PyObject *)obj;
}

struct Mat4Token;
static MemberDefList<Mat4Token> Mat4Members;
static MethodDefList<Mat4Token> Mat4Methods;

struct mat4 {
  PyObject_HEAD;

  ::mat4 m;
};

static int Mat4_Check(PyObject *obj);
static PyObject *Mat4_FromMat4(const ::mat4& m);

#define MAT4_INIT_ERR "argument to mat4() must be number(s) or another mat4"

static int Mat4_Init(mat4 *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);

  if(num_args == 1) {
    auto other = PyTuple_GET_ITEM(args, 0);
    if(Mat4_Check(other)) {
      self->m = ((mat4 *)other)->m;
    } else {
      ArgTypeError(MAT4_INIT_ERR);
      return -1;
    }

    return 1;
  } else {
    float d[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
    };
    if(!PyArg_ParseTuple(args, "|ffffffffffffffff;" VEC4_INIT_ERR,
                         d+0, d+1, d+2, d+3, d+4, d+5, d+6, d+7,
                         d+8, d+9, d+10, d+11, d+12, d+13, d+14, d+15))
      return -1;

    memcpy((float *)self->m, d, sizeof(d));
    return 0;
  }

  return -1; // unreachable
}

static PyObject *Mat4_Repr(mat4 *self)
{
  return Unicode::from_format("mat4(%.2f, %.2f, %.2f, %.2f,\n"
                              "     %.2f, %.2f, %.2f, %.2f,\n"
                              "     %.2f, %.2f, %.2f, %.2f,\n"
                              "     %.2f, %.2f, %.2f, %.2f)",
    self->m.d[0],  self->m.d[1],  self->m.d[2],  self->m.d[3],
    self->m.d[4],  self->m.d[5],  self->m.d[6],  self->m.d[7],
    self->m.d[8],  self->m.d[9],  self->m.d[10], self->m.d[11],
    self->m.d[12], self->m.d[13], self->m.d[14], self->m.d[15]).move();
}

static PyObject *Mat4_Str(mat4 *self)
{
  return Unicode::from_format("[%.2f, %.2f, %.2f, %.2f,\n"
                              " %.2f, %.2f, %.2f, %.2f,\n"
                              " %.2f, %.2f, %.2f, %.2f,\n"
                              " %.2f, %.2f, %.2f, %.2f]",
    self->m.d[0],  self->m.d[1],  self->m.d[2],  self->m.d[3],
    self->m.d[4],  self->m.d[5],  self->m.d[6],  self->m.d[7],
    self->m.d[8],  self->m.d[9],  self->m.d[10], self->m.d[11],
    self->m.d[12], self->m.d[13], self->m.d[14], self->m.d[15]).move();
}

#define MAT4_INDEX_ERR "mat4 element index must be in the range 0-15"

static Py_ssize_t Mat4_Len(mat4 *Py_UNUSED(self))
{
  return 4 * 4;
}

static PyObject *Mat4_Item(mat4 *self, Py_ssize_t item)
{
  if(item < 0 || item > 15) {
    PyErr_SetString(PyExc_IndexError, MAT4_INDEX_ERR);
    return nullptr;
  }

  return PyFloat_FromDouble(self->m.d[item]);
}

static int Mat4_SetItem(mat4 *self, Py_ssize_t item, PyObject *value)
{
  if(item < 0 || item > 15) {
    PyErr_SetString(PyExc_IndexError, MAT4_INDEX_ERR);
    return -1;
  }

  Float f = PyNumber_Float(value);
  if(!f) {
    ArgTypeError("only numbers can be assigned");
    return -1;
  }

  self->m.d[item] = f.f();
  return 0;
}

static PyObject *Mat4_ConstMul(mat4 *self, PyObject *value)
{
  Float u = PyNumber_Float(value);
  if(!u) {
    ArgTypeError("mat4 can only be multiplied with a number (did you mean to use '@'?)");
    return nullptr;
  }

  return Mat4_FromMat4(self->m * (float)u.f());
}

static PyObject *Mat4_MatMul(mat4 *self, PyObject *other)
{
  if(Vec4_Check(other)) {
    auto v = (vec4 *)other;
    return Vec4_FromVec4(self->m * v->m);
  } else if(Mat4_Check(other)) {
    auto m = (mat4 *)other;
    return Mat4_FromMat4(self->m * m->m);
  }

  ArgTypeError("mat4 can only be matrix-multiplied with a vec4 or another mat4");
  return nullptr;
}

static PyObject *Mat4_IMatMul(mat4 *self, PyObject *other)
{
  if(Mat4_Check(other)) {
    self->m *= ((mat4 *)other)->m;
    return (PyObject *)self;
  }

  ArgTypeError("mat4 can only be inplace-matrix-multiplied with a vec4 or another mat4");
  return nullptr;
}

static PyObject *Mat4_Transpose(mat4 *self, PyObject *Py_UNUSED(arg))
{
  return Mat4_FromMat4(self->m.transpose());
}

static PyObject *Mat4_Inverse(mat4 *self, PyObject *Py_UNUSED(arg))
{
  return Mat4_FromMat4(self->m.inverse());
}

static PyNumberMethods Mat4NumberMethods = {
  nullptr, /* nb_add */
  nullptr, /* nb_subtract */
  (binaryfunc)Mat4_ConstMul, /* nb_multiply */
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

  (binaryfunc)Mat4_MatMul,  /* nb_matrix_multiply */
  (binaryfunc)Mat4_IMatMul, /* nb_inplace_matrix_multiply */
};

static PySequenceMethods Mat4SequenceMethods = {
  (lenfunc)Mat4_Len,             /* sq_length */
  nullptr,                       /* sq_concat */
  nullptr,                       /* sq_repeat */
  (ssizeargfunc)Mat4_Item,       /* sq_item */
  nullptr,                       /* sq_slice */
  (ssizeobjargproc)Mat4_SetItem, /* sq_ass_item */
  nullptr,                       /* sq_ass_slice */
  nullptr,                       /* sq_contains */
  nullptr,                       /* sq_inplace_concat */
  nullptr,                       /* sq_inplace_repeat */
};

static TypeObject Mat4_Type = 
  TypeObject()
    .name("mat4")
    .doc("4x4 Matrix indexed like a list (in the range <0; 15>)")
    .size(sizeof(mat4))
    .init((initproc)Mat4_Init)
    .methods(Mat4Methods(
      MethodDef()
        .name("transpose")
        .doc("returns the transposed matrix (rows and columns swapped)")
        .method(Mat4_Transpose)
        .flags(METH_NOARGS),
      MethodDef()
        .name("inverse")
        .doc("returns the inverse of the matrix, it must be non-singular or undefined behaviour will occur")
        .method(Mat4_Inverse)
        .flags(METH_NOARGS)))
    .number_methods(&Mat4NumberMethods)
    .sequence_methods(&Mat4SequenceMethods)
    .repr((reprfunc)Mat4_Repr)
    .str((reprfunc)Mat4_Str)
  ;

static int Mat4_Check(PyObject *obj)
{
  return Mat4_Type.check(obj);
}

static PyObject *Mat4_FromMat4(const ::mat4& m)
{
  auto obj = Mat4_Type.newObject<mat4>();
  obj->m = m;

  return (PyObject *)obj;
}

struct XformToken;
static MethodDefList<XformToken> XformMethods;

static PyObject *Xform_Translate(PyObject *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args) + (kwds ? PyDict_Size(kwds) : 0);
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  float x = 0, y = 0, z = 0;
  if(!PyArg_ParseTupleAndKeywords(args, kwds, "fff:translate",
                                  kwds_names, &x, &y, &z))
    return nullptr;

  return Mat4_FromMat4(xform::translate(x, y, z));
}

static PyObject *Xform_Scale(PyObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(PyTuple_Size(args) == 1 && !kwds) {
    float s = 1;
    if(!PyArg_ParseTuple(args, "f:scale", &s)) return nullptr;

    return Mat4_FromMat4(xform::scale(s));
  } else {
    float x = 1, y = 1, z = 1;
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|fff:scale", kwds_names, &x, &y, &z))
      return nullptr;

    return Mat4_FromMat4(xform::scale(x, y, z));
  }

  return nullptr; // unreachable
}

static PyObject *Xform_Rotate(PyObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(PyTuple_Size(args)) {
    PyErr_SetString(PyExc_ValueError, "rotate() expects only keyword arguments");
    return nullptr;
  }

  float x = 0, y = 0, z = 0;
  if(!PyArg_ParseTupleAndKeywords(args, kwds, "|$fff:rotate", kwds_names, &x, &y, &z))
    return nullptr;

  return Mat4_FromMat4(xform::rotz(z) * xform::roty(y) * xform::rotx(x));
}

struct TransformToken;
static MemberDefList<TransformToken> TransformMembers;
static MethodDefList<TransformToken> TransformMethods;

struct Transform {
  PyObject_HEAD;

  xform::Transform m;
};

static int Transform_Init(Transform *self, PyObject *args, PyObject *kwds)
{
  if(PyTuple_Size(args) || kwds) {
    PyErr_SetString(PyExc_ValueError, "Transform() takes no arguemnts");
    return -1;
  }

  self->m = xform::Transform();
  return 0;
}

static PyObject *Transform_Repr(Transform *self)
{
  return Mat4_Repr((mat4 *)self);
}

static PyObject *Transform_Str(Transform *self)
{
  return Mat4_Str((mat4 *)self);
}

static PyObject *Transform_Translate(Transform *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(num_args == 1 && !kwds) {
    PyObject *v = PyTuple_GET_ITEM(args, 0);
    if(Vec3_Check(v)) {
      self->m.translate(((vec3 *)v)->m);
    } else if(Vec4_Check(v)) {
      self->m.translate(((vec4 *)v)->m);
    } else {
      ArgTypeError("Transform.translate() takes either a vec3/4 or numbers");
      return nullptr;
    }
  } else {
    float x = 0, y = 0, z = 0;
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "fff:Transform_translate", kwds_names, &x, &y, &z))
      return nullptr;

    self->m.translate(x, y, z);
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

static PyObject *Transform_Scale(Transform *self, PyObject *args, PyObject *kwds)
{
  auto num_args = PyTuple_Size(args);
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(num_args == 1 && !kwds) {
    PyObject *v = PyTuple_GET_ITEM(args, 0);
    if(Vec3_Check(v)) {
      self->m.scale(((vec3 *)v)->m);
    } else if(PyNumber_Check(v)) {
      Float f = PyNumber_Float(v);
      self->m.scale(f.f());
    } else {
      ArgTypeError("Transform.scale() takes either a vec3 or number(s)");
      return nullptr;
    }
  } else {
    float x = 0, y = 0, z = 0;
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "fff:Transform_scale", kwds_names, &x, &y, &z))
      return nullptr;

    self->m.scale(x, y, z);
  }

  Py_INCREF(self);
  return (PyObject *)self;
}

static PyObject *Transform_Rotate(Transform *self, PyObject *args, PyObject *kwds)
{
  static char *kwds_names[] = { "x", "y", "z", nullptr };

  if(PyTuple_Size(args) && !kwds) {
    ArgTypeError("Transform.rotate() takes only keyword arguments");
    return nullptr;
  }

  float x = 0, y = 0, z = 0;
  if(!PyArg_ParseTupleAndKeywords(args, kwds, "|$fff:Transform_rotate", kwds_names, &x, &y, &z))
    return nullptr;

  self->m.rotx(x); self->m.roty(y); self->m.rotz(z);

  Py_INCREF(self);
  return (PyObject *)self;
}

#define TRANSFORM_TRANSFORM_ARG_ERR "Transform.transform() takes a single mat4" 

static PyObject *Transform_Transform(Transform *self, PyObject *arg)
{
  if(!Mat4_Type.check(arg)) {
    ArgTypeError(TRANSFORM_TRANSFORM_ARG_ERR);
    return nullptr;
  }
  
  auto m = (mat4 *)arg;
  self->m.transform(m->m);

  Py_INCREF(self);
  return (PyObject *)self;
}

static PyObject *Transform_Matrix(Transform *self, PyObject *Py_UNUSED(arg))
{
  return Mat4_FromMat4(self->m.matrix());
}

static TypeObject Transform_Type = 
  TypeObject()
    .name("Transform")
    .doc("transformation matrix builder")
    .size(sizeof(Transform))
    .init((initproc)Transform_Init)
    .methods(TransformMethods(
      MethodDef()
        .name("translate")
        .doc("applies a translation (takes either a vec3/4 or numbers)")
        .method(Transform_Translate)
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("scale")
        .doc("applies a scale (takes either a vec3 or number(s))")
        .method(Transform_Scale)
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("rotate")
        .doc("applies a rotation in the order X->Y->Z (takes numbers as keyword arguments)")
        .method(Transform_Rotate)
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("transform")
        .doc("applies a transformation (takes a mat4)")
        .method(Transform_Transform)
        .flags(METH_O),
      MethodDef()
        .name("matrix")
        .doc("returns a matrix which will apply the transformations")
        .method(Transform_Matrix)
        .flags(METH_NOARGS)))
    .repr((reprfunc)Transform_Repr)
    .str((reprfunc)Transform_Str)
  ;

static ModuleDef XformModule =
  ModuleDef()
    .name("Math.xform")
    .doc("functions for operating on 4x4 transformation matrices")
    .methods(XformMethods(
      MethodDef()
        .name("translate")
        .doc("returns a 4x4 translation matrix (accepts positional and keyword arguments)")
        .method(Xform_Translate)    
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("scale")
        .doc("returns a 4x4 scale matrix (accepts either a single or 3 positional/keyword arguments)")
        .method(Xform_Scale)
        .flags(METH_VARARGS | METH_KEYWORDS),
      MethodDef()
        .name("rotate")
        .doc("returns a 4x4 rotation matrix ordered like so: X->Y->Z (accepts only keyword arguments)")
        .method(Xform_Rotate)
        .flags(METH_VARARGS | METH_KEYWORDS)))
  ;

static PyObject *Math_Lerp(PyObject *self, PyObject *args)
{
  PyObject *a = nullptr, *b = nullptr;
  float u = 0;

  if(!PyArg_ParseTuple(args, "OOf:lerp", &a, &b, &u))
    return nullptr;

  if(PyNumber_Check(a) && PyNumber_Check(b)) {
    Float fa = PyNumber_Float(a),
      fb = PyNumber_Float(b);

    return PyFloat_FromDouble(lerp(fa.f(), fb.f(), u));
  } else if(Vec4_Check(a) && Vec4_Check(b)) {
    auto va = (vec4 *)a,
      vb = (vec4 *)b;

    return Vec4_FromVec4(lerp(va->m, vb->m, u));
  } else if(Vec3_Check(a) && Vec3_Check(b)) {
    auto va = (vec3 *)a,
      vb = (vec3 *)b;

    return Vec3_FromVec3(lerp(va->m, vb->m, u));
  }

  ArgTypeError("arguments to lerp() must be 2 vectors and a number or 3 numbers");
  return nullptr;
}

static PyObject *Math_Clamp(PyObject *self, PyObject *args)
{
  PyObject *x = nullptr, *minimum = nullptr, *maximum;

  if(!PyArg_ParseTuple(args, "OOO:lerp", &x, &minimum, &maximum))
    return nullptr;

  if(PyNumber_Check(x) && PyNumber_Check(minimum) && PyNumber_Check(maximum)) {
    Float fx = PyNumber_Float(x),
      fminimum = PyNumber_Float(minimum),
      fmaximum = PyNumber_Float(maximum);

    return PyFloat_FromDouble(clamp(fx.f(), fminimum.f(), fmaximum.f()));
  } else if(Vec2_Check(x) && Vec2_Check(minimum) && Vec2_Check(maximum)) {
    auto vx = (vec2 *)x,
      vminimum = (vec2 *)minimum,
      vmaximum = (vec2 *)maximum;

    return Vec2_FromVec2(clamp(vx->m, vminimum->m, vmaximum->m));
  } else if(Vec3_Check(x) && Vec3_Check(minimum) && Vec3_Check(maximum)) {
    auto vx = (vec3 *)x,
      vminimum = (vec3 *)minimum,
      vmaximum = (vec3 *)maximum;

    return Vec3_FromVec3(clamp(vx->m, vminimum->m, vmaximum->m));
  } else if(Vec4_Check(x) && Vec4_Check(minimum) && Vec4_Check(maximum)) {
    auto vx = (vec4 *)x,
      vminimum = (vec4 *)minimum,
      vmaximum = (vec4 *)maximum;

    return Vec4_FromVec4(clamp(vx->m, vminimum->m, vmaximum->m));
  }

  ArgTypeError("arguments to clamp() must be 3 numbers");
  return nullptr;
}

static ModuleDef MathModule = 
  ModuleDef()
    .name("Math")
    .doc("Vector and Matrix types and various 3D graphics related operations.")
    .methods(MathMethods(
      MethodDef()
        .name("lerp")
        .doc("perform linear interpolation: lerp(a, b, u) -> a + (b-a)*u")
        .method(Math_Lerp)
        .flags(METH_VARARGS),
      MethodDef()
        .name("clamp")
        .doc("clamp(x, minimum, maximum) -> clamps x to the range <minimum; maximum>")
        .method(Math_Clamp)
        .flags(METH_VARARGS)))
  ;

PyObject *PyInit_math()
{
  auto self = Module::create(MathModule.py())
    // Submodules
    .addObject("xform",
      Module::create(XformModule.py())
        .addType("Transform", Transform_Type).move()
    )
    
    // Vectors
    .addType("vec2", Vec2_Type)
    .addType("vec3", Vec3_Type)
    .addType("vec4", Vec4_Type)

    // Matrices
    .addType("mat4", Mat4_Type)

    // Constants
    .addObject("Pi", Float(PI))
    ;
        
  return *self;
}

}