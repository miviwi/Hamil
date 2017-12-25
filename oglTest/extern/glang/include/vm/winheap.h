#pragma once

struct IUnknown; // for Clang with microsoft codegen compat

#include <Windows.h>

#include "vm/heap.h"


namespace glang {

class __declspec(dllexport) WinHeap : public IHeap {
public:
  WinHeap();

  virtual byte *base() const;

  virtual long long alloc(size_t sz);
  virtual void free(void *p);

  virtual ~WinHeap();

private:
  HANDLE m_h;
  void *m_base;
};

}