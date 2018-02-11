#pragma once

#include "object.h"
#include "heap.h"

#include <cstdint>
#include <utility>

namespace glang {

class __declspec(dllexport) ObjectManager {
public:
  ObjectManager(IHeap *heap) :
    m_heap(heap)
  {
    m_nil = create<NilObject>();
  }

  Object *nil() const { return m_nil->ref(); }

  ObjectRef ref(ObjectRef r);
  void deref(Object *obj);
  void deref(ObjectRef obj);
 
  IntObject *createInt(long long x);
  FloatObject *createFloat(double x);
  RatioObject *createRatio(rational x);

  StringObject *createString(const char *str);
  SymbolObject *createSymbol(const char *sym);
  KeywordObject *createKeyword(const char *kw);

  VectorObject *createVector(Object *objs[], size_t count);
  MapObject *createMap(Object *keys[], Object *vals[], size_t count);

  HandleObject *createFunction(ICallable *fn);

  IntObject *i(long long x) { return createInt(x); }
  FloatObject *f(double x) { return createFloat(x); }

  StringObject *str(const char *s) { return createString(s); }
  KeywordObject *kw(const char *kw) { return createKeyword(kw); }

  HandleObject *fn(ICallable *fn) { return createFunction(fn); }

private:
  friend class Vm;
  friend class FunctionObject;
  friend class HandleObject;

  void *heapAlloc();
  template <typename T> T *heapAlloc() { return (T *)heapAlloc(); }

  template <typename T, typename... Args>
  T *create(Args... args)
  {
    auto obj = (T *)heapAlloc();
    new(obj) T(std::forward<Args>(args)...);

    return obj;
  }

  Object *ptr(ObjectRef r);
  ObjectRef toRef(Object *o);

  template <typename T>
  T *obj(ObjectRef ref)
  {
    return (T *)ptr(ref);
  }

  Object *box(ObjectRef ref);
  Object *move(ObjectRef ref);

  IntObject asInt(ObjectRef r);
  RatioObject asRatio(ObjectRef r);
  CharObject asChar(ObjectRef r);

  bool compareType(ObjectRef a, const Object& b);
  bool compareType(const Object& a, ObjectRef b) { return compareType(b, a); }

  bool compare(int cond, ObjectRef a, ObjectRef b);

  ObjectRef resultObj(Object::ArtmResult result);

  ObjectRef artm(long op, ObjectRef a, ObjectRef b);
  ObjectRef neg(ObjectRef o);

  ObjectRef doOp(long op, const Object& a, const Object& b);

  IHeap *m_heap;

  Object *m_nil;
};

}