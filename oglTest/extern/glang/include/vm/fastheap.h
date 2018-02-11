#pragma once

#include "vm/heap.h"

namespace glang {

class __declspec(dllexport) FastHeap : public IHeap {
public:
  enum { Size = 1024*1024 };

  FastHeap();

  virtual byte *base() const;

  virtual long long allocBlock();
  virtual void free(void *p);

  virtual ~FastHeap();

private:
  void *m_base;
  byte *m_ptr;
};

}