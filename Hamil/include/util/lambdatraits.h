#pragma once

#include <common.h>

#include <functional>
#include <tuple>

namespace util {

template <typename Fn>
struct LambdaTraitsImpl;

// Source: https://stackoverflow.com/questions/13358672
template <typename Fn, typename Ret, typename... Args>
struct LambdaTraitsImpl<Ret (Fn::*)(Args...) const> {
  using FnType  = std::function<Ret(Args...)>;
  using RetType = Ret;

  using Arguments = std::tuple<Args...>;
  static constexpr auto NumArguments = std::tuple_size_v<Arguments>;

  template <size_t N> using ArgType = std::tuple_element_t<N, Arguments>;

  using Arg1stType = ArgType<0>;

  static FnType to_function(Fn const& fn)
  {
    return fn;
  }
};

// Need specialization for 0-ary functions
template <typename Fn, typename Ret>
struct LambdaTraitsImpl<Ret (Fn::*)() const> {
  using FnType  = std::function<Ret()>;
  using RetType = Ret;

  using Arguments = std::tuple<>;
  static constexpr auto NumArguments = 0;

  static FnType to_function(Fn const& fn)
  {
    return fn;
  }
};

template <typename Fn>
struct LambdaTraits : public LambdaTraitsImpl<
  decltype(&Fn::operator()) /* Look up 'Source' of LambdaTraitsImpl */> {

};

}
