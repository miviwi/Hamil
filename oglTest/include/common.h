#pragma once

#include <cstdint>

typedef unsigned char byte;

typedef unsigned int uint;
typedef unsigned long ulong;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

template <typename T>
class StridePtr {
public:
  StridePtr(T *ptr, size_t stride) :
    m_ptr(ptr), m_stride(stride)
  { }

  T& operator*() { return *m_ptr; }

  StridePtr& operator++()
  {
    m_ptr = (T *)((u8 *)m_ptr + m_stride);
    return *this;
  }
  StridePtr operator++(int)
  {
    StridePtr p(*this);
    operator++();
    return p;
  }

private:
  T *m_ptr;
  size_t m_stride;
};