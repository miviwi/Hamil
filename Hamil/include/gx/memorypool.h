#pragma once

#include <gx/gx.h>

#include <memory>

namespace gx {

// Simple heap used for CommandBuffer operations
//   - Handles returned from alloc() can have arithmetic performed
//     on them and act just like byte* when doing so
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

  struct Error { };
  struct MallocFailedEror : public Error { };

  MemoryPool(size_t size);
  MemoryPool(const MemoryPool& other) = delete;
  ~MemoryPool();
  
  // Returns MemoryPool::Invalid on faliure
  //   - All Handles obtained from alloc() are guaranteed
  //     to be aligned on an AllocAlign-byte boundary
  Handle alloc(size_t sz);

  // Grows the MemoryPool by 'sz' bytes
  //   - Handles remain valid
  //   - Pointers become invalidated
  void grow(size_t sz);

  // See note above grow()
  void resize(size_t sz);

  // Returns a pointer to memory referenced by 'h'
  void *ptr(Handle h);
  template <typename T> T *ptr(Handle h) { return (T *)ptr(h); }

  // Allows all memory owned by the pool to be reused
  //   - ALL handles/pointers become invalidated
  void purge();

  // Allows all memory after 'where' to be reused
  //   and leaves all memory before intact
  // All handles/pointers referencing memory after 
  //   'where' become invalidated
  void purgeFrom(Handle where);

private:
  static uintptr_t align(uintptr_t ptr);

  static void assertHandle(Handle h);

  void checkMalloc();

  byte *m_pool;

  // Start of the pool (m_pool aligned to AllocAlign bytes)
  byte *m_ptr;

  byte *m_rover;
  byte *m_end;
};

}