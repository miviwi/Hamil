#pragma once

#include <common.h>

#include <cstdlib>
#include <new>
#include <utility>
#include <algorithm>
#include <exception>

namespace util {

#pragma pack(push, 1)

template <typename T, size_t N = 32>
class SmallVector {
public:
  static_assert(N - sizeof(u32) >= sizeof(T), "N must be big enugh to hold at least one T!");

  using value_type = T;

  enum {
    InlineSize  = N - sizeof(u32),
    InlineElems = InlineSize / sizeof(T),

    InitialHeapElems = InlineElems < 32 ? 32 : 2*InlineElems,
  };

  using Iterator      = T *;
  using ConstIterator = const T *;

  SmallVector() :
    m_sz(0)
  {
    m_heap.ptr = nullptr;
    m_heap.capacity = 0;
  }

  SmallVector(const SmallVector& other) :
    m_sz(other.m_sz),
    m_heap(other.m_heap)
  {
    if(m_sz > InlineElems) {
      m_heap.ptr = (T *)malloc(m_sz * sizeof(T));
    }

    auto src = other.data();
    auto dst = data();
    for(u32 i = 0; i < m_sz; i++) {
      new(dst + i) T(src[i]);
    }
  }

  ~SmallVector()
  {
    dealloc();
  }

  SmallVector& operator=(const SmallVector& other)
  {
    this->~SmallVector();
    new(this) SmallVector(other);

    return *this;
  }

#if !defined(NDEBUG)
  T& at(u32 idx)
  {
    if(idx >= size()) throw std::runtime_error("Index out of range!");

    return *(data() + idx);
  }
  const T& at(u32 idx) const
  {
    if(idx >= size()) throw std::runtime_error("Index out of range!");

    return *(data() + idx);
  }
#else
  T& at(u32 idx) { return *(data() + idx); }
  const T& at(u32 idx) const { return *(data() + idx); }
#endif

  T& front() { return *data(); }
  const T& front() const { return *data(); }

  T& back() { return *(end() - 1); }
  const T& back() const { return *(cend() - 1); }

  // Appends 'elem' to the end of the container and
  //   returns it's index
  u32 append(const T& elem)
  {
    return emplace(elem);
  }

  // Constructs an element in-place at the end of the
  //   container and returns it's index
  template <typename... Args>
  u32 emplace(Args... args)
  {
    if(m_sz < InlineElems) {
      new(m_inline.data + m_sz) T(std::forward<Args>(args)...);
    } else {
      // Do a heap allocation when there isn't enough capacity or
      //   we're switching from inline storage
      if(m_sz == InlineElems) {
        alloc(InitialHeapElems);
      } else if(m_heap.capacity <= m_sz) {
        alloc((m_sz * 3)/2 /* m_sz * 1.5 */);
      }

      new(m_heap.ptr + m_sz) T(std::forward<Args>(args)...);
    }

    return m_sz++;
  }

  // Removes an element from the end of the container
  //   and returns it
  T pop()
  {
#if !defined(NDEBUG)
    if(empty()) throw std::runtime_error("Called pop() on empty container!");
#endif

    auto ptr = data();
    m_sz--;

    return std::move(*(ptr + m_sz));
  }

  // Sets 'end_ptr' as the new end of the vector
  void resize(u32 *end_ptr)
  {
    m_sz = end_ptr - data();
  }

  void clear()
  {
    // Call destructors
    for(auto it = begin(); it != end(); it++) it->~T();

    m_sz = 0;
  }

  // Shrinks the heap-storage to 'm_sz', when inline
  //   storage is used - does nothing
  void compact()
  {
    if(m_sz <= InlineElems) return;

    alloc(m_sz);
  }

  T *data() const { return (T *)(m_sz > InlineElems ? m_heap.ptr : m_inline.data); }

  T *begin() { return data(); }
  T *end() { return data() + m_sz; }

  const T *cbegin() const { return data(); }
  const T *cend() const { return data() + m_sz; }

  u32 size() const { return m_sz; }

  bool empty() const { return m_sz == 0; }

  u32 capacity() const { return m_sz > InlineElems ? m_heap.capacity : InlineElems; }

private:
  u32 m_sz;
      
  union {
    struct {
      u32 capacity;
      T *ptr;
    } m_heap;

    struct {
      T data[InlineElems];
    } m_inline;
  };

  void alloc(u32 new_sz)
  {
    new_sz = new_sz + (new_sz % 2); // Round to 2

    auto ptr = (T *)malloc(new_sz * sizeof(T)); // avoid initializing the elements
    auto old = data();
    for(u32 i = 0; i < m_sz; i++) {
      new(ptr + i) T(std::move(old[i]));
    }

    if(m_sz > InlineElems) free(m_heap.ptr);

    m_heap.capacity = new_sz;
    m_heap.ptr = ptr;
  }

  void dealloc()
  {
    if(m_sz > InlineElems) {
      free(m_heap.ptr);
    }

    m_heap.ptr = nullptr;
    m_heap.capacity = 0;
  }
};

#pragma pack(pop)

}