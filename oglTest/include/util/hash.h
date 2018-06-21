#pragma once

#include <common.h>

#define XXH_CPU_LITTLE_ENDIAN 1
#include <xxhash/xxhash.hpp>

namespace util {

size_t hash(const void *data, size_t sz);

// Usage:
//   Object a, b;
//   
//   size_t seed = 0;
//   hash_combine(seed, a);
//   hash_combine(seed, b);
//   
//   return seed;
template <typename Hash, typename T>
inline void hash_combine(std::size_t& seed, const T& v)
{
  Hash hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

using Hash = xxh::hash_state64_t;

}