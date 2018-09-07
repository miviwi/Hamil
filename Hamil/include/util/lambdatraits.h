#pragma once

#include <common.h>

#include <functional>

namespace util {

template <typename Fn>
struct LambdaTraitsImpl;

// Source: https://stackoverflow.com/questions/13358672
template <typename Fn, typename Ret, typename Arg>
struct LambdaTraitsImpl<Ret (Fn::*)(Arg) const> {
  using Type    = std::function<Ret(Arg)>;
  using ArgType = Arg;
  using RetType = Ret;

  static Type to_function(Fn const& fn)
  {
    return fn;
  }
};

template <typename Fn>
struct LambdaTraits : public LambdaTraitsImpl<decltype(&Fn::operator())> {

};

}