#pragma once

#include <vm/heap.h>
#include <util/signextend.h>
#include <util/istring.h>
#include <util/stream.h>

#include <cassert>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace glang {

#define GLANG_OBJECT alignas(IHeap::BlockSize)
#define GLANG_ASSERT_OBJECT_SIZE(object) \
 static_assert(sizeof(object) == IHeap::BlockSize, #object " has incorrect size!");
#define GLANG_PAD_OBJECT(num_bytes) char m_pad_[num_bytes];

class Vm;
class ObjectManager;

class NumberObject;
class SequenceObject;

template <typename T>
struct Rational {
  T num, denom;
};

using rational = Rational<long long>;

struct ObjectRef {
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

class Object {
public:
  enum Type  : unsigned {
    Invalid,
    Nil,
    Int, Float, Ratio, Char, String, Symbol, Keyword,
    ConsCell,
    Vector, Map, Set,
    Function,
    Handle,
  };

  enum DebugFlags {
    NoFlags   = 0,

    Temporary = (1<<0),
  };

  Object(Type type_) : Object(type_, NoFlags) { }
  Object(Type type_, DebugFlags flags);
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

  Object *ref() const;
  bool deref() const;

  virtual std::string repr() const = 0;
  virtual bool compare(int cond, const Object& other) const = 0;
  virtual size_t hash() const = 0;
  virtual void finalize(ObjectManager& om) = 0;

  template <typename T>
  T *as() const
  {
    return type() == T::Type ? (T *)this : nullptr;
  }
  template <>
  NumberObject *as() const
  {
    return isNumber() ? (NumberObject *)this : nullptr;
  }
  template <>
  SequenceObject *as() const
  {
    return isSequence() ? (SequenceObject *)this : nullptr;
  }

  struct Hash { size_t operator()(const Object *o) const { return o->hash(); } };
  struct Compare { bool operator()(const Object *a, const Object *b) const; };

  virtual bool isNumber() const { return false; }
  virtual bool isSequence() const { return false; }

  struct ArtmResult {
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
        mutable unsigned m_ref;
      };
  } header;
};

class GLANG_OBJECT NilObject : public Object {
public:
  static constexpr Object::Type Type = Object::Nil;

  NilObject() : Object(Type) { }

  virtual std::string repr() const { return "nil"; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);
  
private:
  friend class Vm;
  friend class ObjectManager;

  GLANG_PAD_OBJECT(16);
};
GLANG_ASSERT_OBJECT_SIZE(NilObject);

class NumberObject : public Object {
public:
  using Object::Object;

  virtual bool isNumber() const { return true; }

  virtual long long toInt() const = 0;
  virtual double toFloat() const = 0;
  virtual rational toRatio() const = 0;
};

class GLANG_OBJECT IntObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Int;

  IntObject(long long data) : NumberObject(Type), m_data(data) { }
  IntObject(long long data, DebugFlags flags) : NumberObject(Type, flags), m_data(data) { }

  virtual std::string repr() const { return std::to_string(m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

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

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(IntObject);

class GLANG_OBJECT FloatObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Float;

  FloatObject(double data) : NumberObject(Type), m_data(data) {}
  FloatObject(double data, DebugFlags flags) : NumberObject(Type, flags), m_data(data) {}

  virtual std::string repr() const { return std::to_string(m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

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

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(FloatObject);

class GLANG_OBJECT RatioObject : public NumberObject {
public:
  static constexpr Object::Type Type = Object::Ratio;

  RatioObject(long long num, long long denom) :
    NumberObject(Type), m_num(num), m_denom(denom) { }
  RatioObject(long long num, long long denom, DebugFlags flags) :
    NumberObject(Type, flags), m_num(num), m_denom(denom) { }

  virtual std::string repr() const
  {
    return std::to_string(m_num)+"/"+std::to_string(m_denom);
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

  virtual long long toInt() const { return m_num/m_denom; }
  virtual double toFloat() const { return (double)m_num/(double)m_denom; }
  virtual rational toRatio() const { return { m_num, m_denom }; }

  rational get() const { return { m_num, m_denom }; }

private:
  friend class Vm;
  friend class ObjectManager;

  long long m_num, m_denom;
};
GLANG_ASSERT_OBJECT_SIZE(RatioObject);

class GLANG_OBJECT CharObject : public Object {
public:
  static constexpr Object::Type Type = Object::Char;

  CharObject(char data) : Object(Type), m_data(data) {}
  CharObject(char data, DebugFlags flags) : Object(Type, flags), m_data(data) {}

  virtual std::string repr() const { return "\\"+std::string(1, m_data); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

private:
  friend class Vm;
  friend class ObjectManager;

  union {
    char m_data;
    long long m_pad;
  };

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(CharObject);

class GLANG_OBJECT StringObject : public Object {
public:
  static constexpr Object::Type Type = Object::String;

  StringObject(size_t size, const char *data) : Object(Type), m_size(size), m_data(data) { }

  virtual std::string repr() const { return "\"" + std::string(m_data) + "\""; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

  const char *get() const { return m_data; }

private:
  friend class Vm;
  friend class ObjectManager;

  size_t m_size;
  const char *m_data;
};
GLANG_ASSERT_OBJECT_SIZE(StringObject);

class SymbolKeywordBase : public Object {
public:
  SymbolKeywordBase(Object::Type type, InternedString str) : Object(type), m_str(str) {}

  virtual std::string repr() const
  {
    const char *prefixes[] = { "'", "" };
    return prefixes[type() == Object::Keyword] + std::string(m_str.ptr());
  }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

  const char *name() const { return m_str.ptr(); }

protected:
  friend class Vm;
  friend class ObjectManager;

  InternedString m_str;

  GLANG_PAD_OBJECT(8);
};

class GLANG_OBJECT SymbolObject : public SymbolKeywordBase {
public:
  static constexpr Object::Type Type = Object::Symbol;

  SymbolObject(InternedString str) : SymbolKeywordBase(Object::Symbol, str) {}
};
GLANG_ASSERT_OBJECT_SIZE(SymbolObject);

class GLANG_OBJECT KeywordObject : public SymbolKeywordBase {
public:
  static constexpr Object::Type Type = Object::Keyword;

  KeywordObject(InternedString str) : SymbolKeywordBase(Object::Keyword, str) {}
};
GLANG_ASSERT_OBJECT_SIZE(KeywordObject);

class GLANG_OBJECT ConsCellObject : public Object {
public:
  static constexpr Object::Type Type = Object::ConsCell;

  ConsCellObject(Object *car_, Object *cdr_) :
    Object(Type), car(car_), cdr(cdr_) { }

  virtual std::string repr() const { return "(" + car->repr() + " " + cdr->repr() + ")"; }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

private:
  friend class Vm;
  friend class ObjectManager;

  Object *car;
  Object *cdr;
};
GLANG_ASSERT_OBJECT_SIZE(ConsCellObject);

class SequenceObject : public Object {
public:
  using Iterator = std::function<void(Object *)>;
  using KeyValueIterator = std::function<void(Object *, Object *)>;

  SequenceObject(Object::Type type) : Object(type) { }

  virtual bool isSequence() const { return true; }

  virtual Object *seqget(Object *key) const = 0;
  template <typename T>
  T *get(long long key) const
  {
    auto key_obj = IntObject(key);
    auto ptr = seqget(&key_obj);
    return ptr ? ptr->as<T>() : nullptr;
  }
  template <typename T>
  T *get(const char *key) const
  {
    auto key_obj = KeywordObject(key);
    auto ptr = seqget(&key_obj);
    return ptr ? ptr->as<T>() : nullptr;
  }
 
  virtual void foreach(Iterator iterator) const = 0;
  virtual void foreachkv(KeyValueIterator iterator) const = 0;
};

class GLANG_OBJECT VectorObject : public SequenceObject {
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
  virtual void finalize(ObjectManager& om);

  virtual Object *seqget(Object *key) const;
  virtual void foreach(Iterator iterator) const;
  virtual void foreachkv(KeyValueIterator iterator) const;

private:
  friend class Vm;

  Vec *m_vec;

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(VectorObject);

class GLANG_OBJECT MapObject : public SequenceObject {
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
  virtual void finalize(ObjectManager& om);

  virtual Object *seqget(Object *key) const;
  // Iterates over values
  virtual void foreach(Iterator iterator) const;
  virtual void foreachkv(KeyValueIterator iterator) const;
  void foreachk(Iterator iterator) const;

private:
  friend class Vm;

  Map *m_map;

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(MapObject);

class GLANG_OBJECT SetObject : public SequenceObject {
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
  virtual void finalize(ObjectManager& om);

  virtual Object *seqget(Object *key) const;
  virtual void foreach(Iterator iterator) const;
  virtual void foreachkv(KeyValueIterator iterator) const;

private:
  friend class Vm;

  Set *m_set;

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(SetObject);

class GLANG_OBJECT FunctionObject : public Object {
public:
  static constexpr Object::Type Type = Object::Function;

  FunctionObject(void *p) : Object(Type), m_p((pFn *)p) { }

  virtual std::string repr() const { return fmt("#<fn@0x%x>", (long long)m_p->code); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

private:
  friend class Vm;
  friend class ObjectManager;

  struct pFn {
    unsigned char num_args, num_locals, num_upvalues, var_args;

    ObjectRef *upvalues;
    void *code;
  };

  pFn *m_p;

  GLANG_PAD_OBJECT(8);
};
GLANG_ASSERT_OBJECT_SIZE(FunctionObject);

class ICallable {
public:
  virtual Object *call(ObjectManager& om, Object *args[], size_t num_args) = 0;
};

class GLANG_OBJECT HandleObject : public Object {
public:
  enum Tag {
    Invalid,
    Callable, Ptr,
  };

  static constexpr Object::Type Type = Object::Handle;

  HandleObject(Tag tag, void *ptr) : Object(Type), m_tag(tag), m_ptr(ptr) { }

  virtual std::string repr() const { return fmt("#<handle@0x%x>", (long long)m_ptr); }
  virtual bool compare(int cond, const Object& other) const;
  virtual size_t hash() const;
  virtual void finalize(ObjectManager& om);

  static HandleObject *create(ObjectManager& om, Tag tag, void *ptr);
  static void destroy(ObjectManager& om, HandleObject *handle);

  unsigned tag() const { return m_tag; }

  void *get() const { return m_ptr; }

private:
  friend class Vm;
  friend class ObjectManager;

  Tag m_tag;

  union {
    ICallable *m_fn;
    void *m_ptr;
  };
};
GLANG_ASSERT_OBJECT_SIZE(HandleObject);

}