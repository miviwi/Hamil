#pragma once

#include <vm/object.h>
#include <vm/objman.h>

#include <tuple>
#include <functional>
#include <type_traits>

namespace glang {

namespace object_traits {

template <typename T>
struct Traits { static bool convertableFrom(Object *o) { return false; } };

template <>
struct Traits<NumberObject> { static bool convertableFrom(Object *o) { return o->isNumber(); } };

template <>
struct Traits<IntObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Int; } };

template <>
struct Traits<FloatObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Float; } };

template <>
struct Traits<RatioObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Ratio; } };

template <>
struct Traits<CharObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Char; } };

template <>
struct Traits<StringObject> { static bool convertableFrom(Object *o) { return o->type() == Object::String; } };

template <>
struct Traits<SymbolObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Symbol; } };

template <>
struct Traits<KeywordObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Keyword; } };

template <>
struct Traits<ConsCellObject> { static bool convertableFrom(Object *o) { return o->type() == Object::String; } };

template <>
struct Traits<VectorObject> { static bool convertableFrom(Object *o) { return o->type() == Object::String; } };

template <>
struct Traits<MapObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Map; } };

template <>
struct Traits<FunctionObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Function; } };

template <>
struct Traits<HandleObject> { static bool convertableFrom(Object *o) { return o->type() == Object::Handle; } };

}

template <typename T>
T *p_get_ptr(Object *p)
{
  using Traits = object_traits::Traits<T>;
  return Traits::convertableFrom(p) ? (T *)p : nullptr;
}

template <typename... Args, size_t... Inds>
auto p_parse_args(Object *args[], std::index_sequence<Inds...>)
{
  return std::make_tuple(p_get_ptr<Args>(args[Inds])...);
}

template <typename... Args>
std::tuple<std::add_pointer_t<Args>...> parse_args(Object *args[])
{
  return p_parse_args<Args...>(args, std::index_sequence_for<Args...>());
}

template <typename... Args>
class CallableFn : public ICallable {
public:
  using Fn = std::function<Object *(ObjectManager&, std::tuple<std::add_pointer_t<Args>...>)>;

  CallableFn(Fn fn) : m_fn(fn) { }

  virtual Object *call(ObjectManager& om, Object *args[], size_t num_args)
  {
    if(num_args < sizeof...(Args)) return om.nil();

    return m_fn(om, parse_args<Args...>(args));
  }

private:
  Fn m_fn;
};

template <>
class CallableFn<> : public ICallable {
public:
  using Fn = std::function<Object *(ObjectManager&)>;

  CallableFn(Fn fn) : m_fn(fn) { }

  virtual Object *call(ObjectManager& om, Object *args[], size_t num_args)
  {
    if(num_args) return om.nil();
    return m_fn(om);
  }

private:
  Fn m_fn;
};

}