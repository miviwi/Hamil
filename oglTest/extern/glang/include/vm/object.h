#pragma once

#include "util/signextend.h"
#include "util/istring.h"
#include "util/stream.h"

#include <cassert>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>

namespace glang {

class IHeap;
class Vm;
class ObjectManager;

template <typename T>
struct Rational {
  T num, denom;
};

using rational = Rational<long long>;

struct __declspec(dllexport) ObjectRef {
  typedef unsigned long id;

  enum : id {
    Inline = 1,
    Nil = 0, Int = 1, Char = 2, Ratio = 3,

    FlagsMask   = 0x07,
    TypeMask    = (0x03<<1),
    TypeShift   = 1,

    ValueMask   = 0xFFFFFFF8,
    MaxValue    = 0x1FFFFFFF,
    ValueShift  = 3,
    ValueBits   = sizeof(id)*8 - ValueShift,
    HiValShift  = (ValueBits/2)+1,
    HiValBits   = ValueBits/2,
    RatioValMax = 0x00003FFF,

    PtrMask     = 0xFFFFFFFE,
    MaxPtr      = 0x7FFFFFFF,
    PtrShift    = 1,
  };

  static ObjectRef nil()
  {
    return ObjectRef{ Inline|(Nil<<TypeShift) };
  }

  static ObjectRef small_int(long x)
  {
    assert(abs(x) <= MaxValue && "small_int exceeds max value!");

    x &= MaxValue;
    return ObjectRef{ Inline|(Int<<TypeShift) | (x<<ValueShift) };
  }

  static ObjectRef char_(char ch)
  {
    return ObjectRef{ Inline|(Char<<TypeShift) | (ch<<ValueShift) };
  }

  static ObjectRef ratio(long num, long denom)
  {
    assert(abs(num) <= RatioValMax && abs(denom) <= RatioValMax && "num or denom exceed max value!");

    num &= RatioValMax; denom &= RatioValMax;
    return ObjectRef{ Inline|(Ratio<<TypeShift) | (num<<HiValShift) | (denom<<(ValueShift+1)) };
  }

  static ObjectRef heap_object(id ptr)
  {
    assert(ptr <= MaxPtr && "ptr exceeds max value!");

    return ObjectRef{ (ptr<<PtrShift) };
  }

  id raw() const { return m_id; }

  id isInline() const { return (m_id&Inline); }
  id type() const { return (m_id&TypeMask)>>TypeShift; }

  long long val() const { return sign_extend<long long, ValueBits>((m_id&ValueMask)>>ValueShift); }
  long long hival() const { return sign_extend<long long, HiValBits>(val()>>HiValShift); }
  long long loval() const { return sign_extend<long long, HiValBits>(val()>>1); }

  id ptr() const { return (m_id&PtrMask)>>PtrShift; }

  ObjectRef() { *this = nil(); }

private:
  ObjectRef(id id_) : m_id(id_) { }

  id m_id;
};

class __declspec(dllexport) Object {
public:
  enum Type {
    Invalid,
    Nil,
    Int, Float, Ratio, Char, String, Symbol, Keyword,
    ConsCell,
    Vector, Map, Set,
    Function,
    Handle,
  };

  Object(Type type_) : header({ type_ }) {}

  virtual ~Object() { }

  Type type() const { return header.m_type; }

  const char *typeLiteral() const
  {
    static const char *lits[] = {
      "Invalid",
      "Nil",
      "Int", "Float", "Ratio", "Char",  "String", "Symbol", "Keyword",
      "ConsCell",
      "Vector", "Map", "Set",
      "Function",
      "Handle",
    };
    return lits[header.m_type];
  }

  virtual std::string repr() const = 0;
  virtual bool compare(int cond, const Object& other) const = 0;
  virtual size_t hash() const = 0;
  virtual void finalize(IHeap *heap) = 0;

  struct __declspec(dllexport) Hash { size_t operator()(const Object *o) const { return o->hash(); } };
  struct __declspec(dllexport) Compare { bool operator()(const Object *a, const Object *b) const; };

  virtual bool isNumber() const { return false; }

  struct __declspec(dllexport) ArtmResult {
    enum Tag { Invalid, Int, Float, Ratio, } tag;

    union {
      long long i;
      double f;
      rational r;
    };

    ArtmResult() : tag(Invalid) { }
    ArtmResult(long long i_) : tag(Int), i(i_) { }
    ArtmResult(double f_) : tag(Float), f(f_) { }
    ArtmResult(long long num_, long long denom_) : tag(Ratio), r({ num_, denom_ }) { }
  };

  virtual ArtmResult add(const Object& o) const { return ArtmResult(); }
  virtual ArtmResult sub(const Object& o) const { return ArtmResult(); }
  virtual ArtmResult mul(const Object& o) const { return ArtmResult(); }
  virtual ArtmResult div(const Object& o) const { return ArtmResult(); }

  virtual ArtmResult neg() { return ArtmResult(); }

protected:
  friend class Vm;
  friend class ObjectManager;

  union Header {
      long long raw_header;

      struct {
        Type m_type;
      };
  } header;
};

class alignas(32) __declspec(dllexport) NilObject : public Object {
public:
  static constexpr Object::Type Type = Object::Nil;

  NilObject() : Object(Type) { }

  virtual std::string repr() const { return "nil"; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);
  
private:
  friend class Vm;
  friend class ObjectManager;
};

class __declspec(dllexport) NumberObject : public Object {
public:
  NumberObject(Object::Type type) : Object(type) { }

  virtual bool isNumber() const { return true; }

  virtual long long toInt() const = 0;
  virtual double toFloat() const = 0;
  virtual rational toRatio() const = 0;
};

class alignas(32) __declspec(dllexport) IntObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Int;

  IntObject(long long data) : NumberObject(Type), m_data(data) { }

  virtual std::string repr() const { return std::to_string(m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual ArtmResult add(const Object& o) const;
  virtual ArtmResult sub(const Object& o) const;
  virtual ArtmResult mul(const Object& o) const;
  virtual ArtmResult div(const Object& o) const;

  virtual ArtmResult neg();

  virtual long long toInt() const { return m_data; }
  virtual double toFloat() const { return (double)m_data; }
  virtual rational toRatio() const { return { m_data, 1 }; }

  long long get() const { return m_data; }

private:
  friend class Vm;
  friend class ObjectManager;

  long long m_data;
};

class alignas(32) __declspec(dllexport) FloatObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Float;

  FloatObject(double data) : NumberObject(Type), m_data(data) {}

  virtual std::string repr() const { return std::to_string(m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual ArtmResult add(const Object& o) const;
  virtual ArtmResult sub(const Object& o) const;
  virtual ArtmResult mul(const Object& o) const;
  virtual ArtmResult div(const Object& o) const;

  virtual ArtmResult neg();

  virtual long long toInt() const { return (long long)m_data; }
  virtual double toFloat() const { return m_data; }
  virtual rational toRatio() const { return { (long long)m_data, 1 }; }

  double get() const { return m_data; }

private:
  friend class Vm;
  friend class ObjectManager;

  double m_data;
};

class alignas(32) __declspec(dllexport) RatioObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Ratio;

  RatioObject(long long num, long long denom) :
    NumberObject(Type), m_num(num), m_denom(denom) { }

  virtual std::string repr() const
  {
    return std::to_string(m_num)+"/"+std::to_string(m_denom);
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual long long toInt() const { return m_num/m_denom; }
  virtual double toFloat() const { return (double)m_num/(double)m_denom; }
  virtual rational toRatio() const { return { m_num, m_denom }; }

  rational get() const { return { m_num, m_denom }; }

private:
  friend class Vm;
  friend class ObjectManager;

  long long m_num, m_denom;
};

class alignas(32) __declspec(dllexport) CharObject : public Object {
public:
  static constexpr Object::Type Type = Object::Char;

  CharObject(char data) : Object(Type), m_data(data) {}

  virtual std::string repr() const { return "\\"+std::string(1, m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

private:
  friend class Vm;
  friend class ObjectManager;

  union {
    char m_data;
    long long m_pad;
  };
};

class alignas(32) __declspec(dllexport) StringObject : public Object {
public:
  static constexpr Object::Type Type = Object::String;

  StringObject(void *p) : Object(Type), m_p((pStr *)p) { }

  virtual std::string repr() const { return "\"" + std::string(m_p->data) + "\""; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  const char *get() const { return m_p->data; }

private:
  friend class Vm;
  friend class ObjectManager;

  struct alignas(32) pStr {
    size_t size;
    const char *data;
  };

  pStr *m_p;
};

class __declspec(dllexport) SymbolKeywordBase : public Object {
public:
  SymbolKeywordBase(Object::Type type, InternedString str) : Object(type), m_str(str) {}

  virtual std::string repr() const
  {
    const char *prefixes[] = { "'", "" };
    return prefixes[type() == Object::Keyword] + std::string(m_str.ptr());
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  const char *name() const { return m_str.ptr(); }

protected:
  friend class Vm;
  friend class ObjectManager;

  InternedString m_str;
};

class alignas(32) __declspec(dllexport) SymbolObject : public SymbolKeywordBase {
public:
  SymbolObject(InternedString str) : SymbolKeywordBase(Object::Symbol, str) {}
};

class alignas(32) __declspec(dllexport) KeywordObject : public SymbolKeywordBase {
public:
  KeywordObject(InternedString str) : SymbolKeywordBase(Object::Keyword, str) {}
};

class alignas(32) __declspec(dllexport) ConsCellObject : public Object {
public:
  static constexpr Object::Type Type = Object::ConsCell;

  ConsCellObject(Object *car_, Object *cdr_) :
    Object(Type), car(car_), cdr(cdr_) { }

  virtual std::string repr() const { return "(" + car->repr() + " " + cdr->repr() + ")"; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

private:
  friend class Vm;
  friend class ObjectManager;

  Object *car;
  Object *cdr;
};

class __declspec(dllexport) SequenceObject : public Object {
public:
  SequenceObject(Object::Type type) : Object(type) { }

  virtual Object *seqget(Object *key) const = 0;
};

class alignas(32) __declspec(dllexport) VectorObject : public SequenceObject {
public:
  static constexpr Object::Type Type = Object::Vector;

  using Vec = std::vector<Object *>;

  VectorObject(Vec *vec) : SequenceObject(Type), m_vec(vec) { }

  virtual std::string repr() const
  {
    std::string r = "[";
    for(auto& e : *m_vec) r += e->repr() + " ";
    r.pop_back();
    return r + "]";
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual Object *seqget(Object *key) const;

private:
  friend class Vm;

  Vec *m_vec;
};

class alignas(32) __declspec(dllexport) MapObject : public SequenceObject {
public:
  static constexpr Object::Type Type = Object::Map;

  using Map = std::unordered_map<Object *, Object *, Object::Hash, Object::Compare>;

  MapObject(Map *map) : SequenceObject(Type), m_map(map) { }

  virtual std::string repr() const
  {
    std::string r = "{";
    for(auto& e : *m_map) r += e.first->repr() + " " + e.second->repr() + ", ";
    return r.substr(0, r.size()-2) + "}";
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual Object *seqget(Object *key) const;

private:
  friend class Vm;

  Map *m_map;
};

class alignas(32) __declspec(dllexport) SetObject : public SequenceObject {
public:
  static constexpr Object::Type Type = Object::Set;

  using Set = std::unordered_set<Object *, Object::Hash, Object::Compare>;

  SetObject(Set *set) : SequenceObject(Type), m_set(set) { }

  virtual std::string repr() const
  {
    std::string r = "#{";
    for(auto& e : *m_set) r += e->repr() + ", ";
    return r.substr(0, r.size()-2) + "}";
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  virtual Object *seqget(Object *key) const;

private:
  friend class Vm;

  Set *m_set;
};

class alignas(32) __declspec(dllexport) FunctionObject : public Object {
public:
  static constexpr Object::Type Type = Object::Function;

  FunctionObject(void *p) : Object(Type), m_p((pFn *)p) { }

  virtual std::string repr() const { return fmt("#<fn@0x%x>", (long long)m_p->code); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

private:
  friend class Vm;
  friend class ObjectManager;

  struct alignas(32) pFn {
    unsigned char num_args, num_locals, num_upvalues, var_args;

    ObjectRef *upvalues;
    void *code;
  };

  pFn *m_p;
};

class ICallable {
public:
  virtual Object *call(ObjectManager& om, Object *args[], size_t num_args) = 0;
};

class alignas(32) __declspec(dllexport) HandleObject : public Object {
public:
  enum Tag {
    Invalid,
    Callable, Ptr,
  };

  static constexpr Object::Type Type = Object::Handle;

  HandleObject(void *p) : Object(Type), m_p((pHandle *)p) { }

  virtual std::string repr() const { return fmt("#<handle@0x%x>", (long long)m_p->ptr); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(IHeap *heap);

  static HandleObject *create(IHeap *heap, Tag tag, void *ptr);
  static void destroy(IHeap *heap, HandleObject *handle);

  unsigned tag() const { return m_p->tag; }

  void *get() const { return m_p->ptr; }

private:
  friend class Vm;
  friend class ObjectManager;

  struct alignas(32) pHandle {
    Tag tag;

    union {
      ICallable *fn;
      void *ptr;
    };
  };

  pHandle *m_p;
};

}