#pragma once

struct IUnknown; // for Clang with microsoft codegen compat

#include <Windows.h>

#include <vm/heap.h>


namespace glang {

class WinHeap : public IHeap {
public:
  enum { InitialSize = 1024*1024 };

  WinHeap();

  virtual byte *base() const;

  virtual long long allocBlock();
  virtual void free(void *p);

  virtual ~WinHeap();

private:
  HANDLE m_h;
  void *m_base;
};

}