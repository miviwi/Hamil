#pragma once

#include <py/python.h>

template <typename T>
struct Vector2;

template <typename T>
struct Vector3;

template <typename T>
struct Vector4;

namespace xform {
class Transform;
}

namespace py {

PyObject *PyInit_math();

int Vec2_Check(PyObject *obj);
PyObject *Vec2_FromVec2(Vector2<float> v);
Vector2<float> Vec2_AsVec2(PyObject *obj);

int Vec3_Check(PyObject *obj);
PyObject *Vec3_FromVec3(Vector3<float> v);
Vector3<float> Vec3_AsVec3(PyObject *obj);

int Vec4_Check(PyObject *obj);
PyObject *Vec4_FromVec4(Vector4<float> v);
Vector4<float> Vec4_AsVec4(PyObject *obj);

int Transform_Check(PyObject *obj);
PyObject *Transform_FromTransform(const xform::Transform& transform);
xform::Transform Transform_AsTransform(PyObject *obj);

}