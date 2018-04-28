#pragma once

#include <cstdint>

namespace glang {

typedef unsigned char byte;

class IHeap {
public:
  enum {
    BlockSize = 32,

    BlockBits = 5,
    BlockMask = BlockSize-1,
  };

  virtual ~IHeap() { }

  static void *block_align(void *ptr)
  {
    auto p = (uintptr_t)ptr;
    if(p & BlockMask) p += (BlockSize - (p & BlockMask));

    return (void *)p;
  }

  virtual byte *base() const = 0;
  virtual long long allocBlock() = 0;
  virtual void free(void *p) = 0;
};

}
