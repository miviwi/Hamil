#pragma once

#include <cstdint>

namespace glang {

typedef unsigned char byte;

class __declspec(dllexport) IHeap {
public:
  virtual ~IHeap() { }

  virtual byte *base() const = 0;
  virtual long long alloc(size_t sz) = 0;
  virtual void free(void *p) = 0;
};

}
