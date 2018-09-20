#pragma once

#include <common.h>

#include <new>
#include <utility>
#include <algorithm>

#include <cstdlib>

namespace util {

#pragma pack(push, 1)

template <typename T, size_t N = 32>
class SmallVector {
public:
  SmallVector() :
    m_sz(0)
  {
    m_heap.ptr = nullptr;
    m_heap.capacity = 0;
  }

  ~SmallVector()
  {
    dealloc();
  }

  // Appends 'elem' to the end of the container and
  //   returns it's index
  u32 append(const T& elem)
  {
    auto index = m_sz;
    m_sz++;

    if(index < InlineElems) {
      new(m_inline.data + index) T(elem);
    } else {
      // Do a heap allocation when there isn't enough capacity or
      //   we're switching from inline storage
      if(index == InlineElems || m_heap.capacity <= index) {
        alloc((m_sz * 3)/2 /* m_sz * 1.5 */);
      }

      new(m_heap.ptr + index) T(elem);
    }

    return index;
  }

  // Removes an element from the end of the container
  //   and returns it
  T pop()
  {
    auto ptr = data();
    m_sz--;

    return std::move(ptr + m_sz);
  }

  // Sets 'end_ptr' as the new end of the vector
  void erase(u32 *end_ptr)
  {
    m_sz = end_ptr - data();
  }

  // Shrinks the heap-storage to 'm_sz', when inline
  //   storage is used - does nothing
  void compact()
  {
    if(m_sz <= InlineElems) return;

    // TODO
  }

  T *begin() { return data(); }
  T *end() { return data() + m_sz; }

  const T *cbegin() const { return data(); }
  const T *cend() const { return data() + m_sz; }

  u32 size() const { return m_sz; }

private:
  enum {
    InlineSize  = N - sizeof(u32),
    InlineElems = InlineSize / sizeof(T),
  };

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

  T *data() const { return (T *)(m_sz > InlineElems ? m_heap.ptr : m_inline.data); }

  void alloc(u32 new_sz)
  {
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