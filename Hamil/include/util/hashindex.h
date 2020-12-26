#pragma once

#include <common.h>

#include <vector>
#include <limits>

namespace util {

class HashIndex {
public:
  using Key   = u32;
  using Index = u32;

  enum : size_t {
    InitialHashSize        = 1024,
    InitialHashGranularity = 1024,
  };

  enum : Index {
    Invalid = std::numeric_limits<Key>::max(),
  };

  HashIndex();
  HashIndex(size_t hash_sz, size_t chain_sz);
  HashIndex(const HashIndex& other);
  HashIndex(HashIndex&& other);
  ~HashIndex();

  HashIndex& operator=(const HashIndex& other);
  HashIndex& operator=(HashIndex&& other);

  void resize(size_t chain_sz);

  void clear();

  void add(Key key, Index idx);
  void remove(Key key, Index idx);

  Index first(Key key) const;
  Index next(Index idx) const;

  template <typename Fn>
  [[using gnu: always_inline]]
  Index find(Key key, Fn compare)
  {
    for(auto index = first(key); index != Invalid; index = next(index)) {
      if(compare(key, index)) return index;
    }

    return Invalid;
  }

  size_t hashSize() const;
  size_t chainSize() const;

  unsigned spread() const;

private:
  enum : Key {
    NoLookup = 0,
    DoLookup = std::numeric_limits<Key>::max(),
  };

  void init(size_t hash_sz, size_t chain_sz);

  void alloc(size_t hash_sz, size_t chain_sz);
  void dealloc();

  size_t m_hash_sz = 0;
  Index *m_hash = nullptr;

  size_t m_chain_sz = 0;
  Index *m_chain = nullptr;

  size_t m_granularity;

  Key m_hash_mask;
  Key m_lookup_mask;

  static Index InvalidIndex[1];
};

}
