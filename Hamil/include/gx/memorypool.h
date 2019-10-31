#pragma once

#include <gx/gx.h>

#include <climits>

#include <atomic>
#include <memory>

namespace gx {

// Simple heap used for CommandBuffer operations
//   - Doesn't allow freeing of individual allocations
//   - Handles returned from alloc() can have arithmetic performed
//     on them and act just like byte* when doing so
//   - alloc() is thread-safe via atomic operations
//   - resize() is NOT thread-safe!
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
  
  // Returns MemoryPool::Invalid on failure
  //   - All Handles obtained from alloc() are guaranteed
  //     to be aligned on an AllocAlign-byte boundary
  Handle alloc(size_t sz);
  template <typename T> Handle alloc() { return alloc(sizeof(T)); }

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

  size_t size() const;

private:
  enum {
    // Added to the 'size' passed to the constructor
    //   so alignment bytes don't cause out-of-bounds
    //   memory accesses
    MaxSizeDefficit = AllocAlign,
  };

  static uintptr_t align(uintptr_t ptr);

  // Asserts if 'h' != Invalid
  static void assertHandle(Handle h);

  // Throws MallocFailedError when malloc() returns nullptr
  void checkMalloc();

  byte *m_pool;

  // Start of the pool (m_pool aligned to AllocAlign bytes)
  byte *m_ptr;

  // alloc() uses std::atomic::compare_exchange_strong()
  //   to ensure thread-safety
  std::atomic<byte *> m_rover;
  byte *m_end;
};

}
