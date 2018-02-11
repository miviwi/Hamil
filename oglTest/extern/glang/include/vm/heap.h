#pragma once

#include <cstdint>

namespace glang {

typedef unsigned char byte;

class __declspec(dllexport) IHeap {
public:
  enum {
    BlockSize = 32,
  };

  virtual ~IHeap() { }

  virtual byte *base() const = 0;
  virtual long long allocBlock() = 0;
  virtual void free(void *p) = 0;
};

}
