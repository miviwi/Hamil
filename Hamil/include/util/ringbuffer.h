#pragma once

#include <common.h>

#include <type_traits>

template <typename T, size_t N = 8>
class RingBuffer {
public:
  static_assert(
    std::is_default_constructible<T>::value && std::is_trivially_destructible<T>::value,
                "T must have default constructor and destructor in RingBuffer!");

  RingBuffer() { clear(); }
  
  void push(T elem)
  {
    m_data[m_ptr] = elem;

    m_empty = false;
    m_ptr = (m_ptr+1) % N;
  }

  T last()
  {
    size_t ptr = (m_ptr-1) % N;

    return m_data[ptr];
  }

  T pop()
  {
    auto r = last();

    m_ptr = (m_ptr-1) % N;

    return r;
  }

  bool empty() const { return m_empty; }

  void clear()
  {
    m_empty = true;
    m_ptr = 0;
  }

private:
  bool m_empty;
  size_t m_ptr;
  T m_data[N];
};
