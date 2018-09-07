#pragma once

#include <common.h>

#include <tuple>

namespace util {

// Source: https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type

template <typename T, typename Tuple>
struct TupleIndex;

template <typename T, typename... Types>
struct TupleIndex<T, std::tuple<T, Types...>> {
  static const std::size_t value = 0;
};

template <typename T, typename U, typename... Types>
struct TupleIndex<T, std::tuple<U, Types...>> {
  static const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
};

template <typename T, typename Tuple>
constexpr size_t tuple_index_v = TupleIndex<T, Tuple>::value;

}