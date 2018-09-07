#pragma once

#include <common.h>

#include <vector>

namespace util {

class BitVector {
public:
  using Elem = u64;

  enum : size_t {
    ElemBits     = sizeof(Elem)*CHAR_BIT,
    ElemShift    = 6,
    ElemMask     = (1<<ElemShift)-1,
    InitialAlloc = 0x1000,
  };

  BitVector(size_t alloc = InitialAlloc);

  bool get(size_t idx) const;

  // Sets the bit at 'idx' (makes it == 1)
  void set(size_t idx);
  // Clears the bit at 'idx' (makes it == 0)
  void clear(size_t idx);

  // When 'val' == 1 sets the bit at 'idx',
  //   otherwise clears it
  // Resizes the underlying container if it's too small
  void put(size_t idx, bool val);

  // Appends 'val' and returns it's index
  size_t append(bool val);

  void resize(size_t new_size);

  // Returns the number of BITS stored in the vector
  size_t size() const;

private:
  size_t m_sz;
  std::vector<Elem> m;
};

}