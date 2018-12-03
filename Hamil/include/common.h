#pragma once

#include <cstdint>
#include <cassert>

typedef unsigned char byte;

typedef intptr_t ssize_t;

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define STUB() assert(0 && "stub " __FUNCSIG__ "!");

#define INTRIN_INLINE __forceinline

template <typename T>
class StridePtr {
public:
  StridePtr(void *ptr, size_t stride) :
    m_ptr((T *)ptr), m_stride(stride)
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

  T *get() const { return m_ptr; }

private:
  T *m_ptr;
  size_t m_stride;
};