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
  template <size_t N> using ArgType = std::tuple_element_t<N, Arguments>;

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