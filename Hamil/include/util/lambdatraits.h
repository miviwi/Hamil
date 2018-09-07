#pragma once

#include <common.h>

#include <functional>

namespace util {

template <typename Fn>
struct LambdaTraits;

template <typename Fn, typename Arg>
struct LambdaTraits<void (Fn::*)(Arg) const> {
  using Type    = std::function<void(Arg)>;
  using ArgType = Arg;

  static Type to_function(Fn const& fn)
  {
    return fn;
  }
};

}