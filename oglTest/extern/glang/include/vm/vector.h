#pragma once

#include <cstdint>

namespace glang {

class Object;

struct VectorBuffer {
  VectorBuffer(size_t sz);
  VectorBuffer(const VectorBuffer& other) = delete;
  ~VectorBuffer();

  size_t size() const;

  VectorBuffer *ref();
  bool deref();

  Object **begin;
  Object **end;

private:
  unsigned m_ref;
};

struct Vector {
  Vector(VectorBuffer *self);
  Vector(const Vector& other) = delete;
  Vector(VectorBuffer *self, Vector *next);
  ~Vector();

  Vector& operator=(const Vector& other) = delete;

  Object *get(size_t index) const;

  void ref();

private:
  VectorBuffer *m_self;
  Vector *m_next;
};

class VectorSlice {
  VectorSlice(Vector *self_);
  ~VectorSlice();

  Object *get(unsigned index) const;

  Vector *self;
  unsigned first, last;
};

}