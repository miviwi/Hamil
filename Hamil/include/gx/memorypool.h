#pragma once

#include <gx/gx.h>

#include <memory>

namespace gx {

class MemoryPool {
public:
  using u32 = ::u32;
  using Handle = u32;

  enum : size_t {
    AllocAlign = 32,

    AllocAlignBits  = 5,
    AllocAlignShift = AllocAlignBits,
    AllocAlignMask  = (1<<AllocAlignBits) - 1,

    HandleBits = sizeof(u32)*CHAR_BIT,
    HandleMask = (1ull<<HandleBits) - 1ull,
  };

  enum : Handle {
    Invalid = ~0u,
  };

  MemoryPool(size_t size);
  
  Handle alloc(size_t sz);

  void *ptr(Handle h);
  template <typename T> T *ptr(Handle h) { return (T *)ptr(h); }

  // Resets the rover
  void purge();

private:
  static uintptr_t align(uintptr_t ptr);

  std::shared_ptr<byte[]> m_pool;

  // Start of the pool (m_pool aligned to AllocAlign bytes)
  byte *m_ptr;

  byte *m_rover;
  byte *m_end;
};

}