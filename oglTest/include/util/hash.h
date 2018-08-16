#pragma once

#include <common.h>

#define XXH_CPU_LITTLE_ENDIAN 1
#include <xxhash/xxhash.hpp>

namespace util {

size_t hash(const void *data, size_t sz);

template <typename T>
struct XXHash {
  size_t operator()(const T& x) const
  {
    return xxh::xxhash<64>(x);
  }
};

struct ByteXXHash {
  const size_t len;
  ByteXXHash(size_t len_) : len(len_) { }

  size_t operator()(const void *ptr) const
  {
    return xxh::xxhash<64>(ptr, len);
  }
};

// Usage:
//   Object a, b;
//   
//   size_t seed = 0;
//   hash_combine(seed, a);
//   hash_combine(seed, b);
//   
//   return seed;
template <typename Hash, typename T>
inline void hash_combine(const Hash& hasher, size_t& seed, const T& v)
{
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <typename Hash, typename T>
inline void hash_combine(size_t& seed, const T& v)
{
  return hash_combine(Hash(), seed, v);
}

}