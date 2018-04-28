#pragma once

#include <cstdint>
#include <list>
#include <utility>

class FreeListAllocator {
public:
  using Range = std::pair<size_t, size_t>;

  enum : size_t {
    Error = ~0llu,
  };

  FreeListAllocator(size_t sz);

  size_t alloc(size_t sz);
  void dealloc(size_t offset, size_t sz);

private:
  size_t allocAtEnd(size_t sz);
  void coalesce();

  size_t m_sz;
  size_t m_ptr;
  std::list<Range> m_free_list;
};