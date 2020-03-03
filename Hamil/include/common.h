#pragma once

#include <cstddef>
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

// Helper macros
#define STR(x) #x
#define STRINGIFY(x) STR(x)

#if defined(_MSVC_VER)
#  define STUB() do { assert(0 && "stub " STRINGIFY(__FUNCSIG__) " called!"); } while(0)
#else
#  define STUB() do { assert(0 && "stub " STRINGIFY(__PRETTY_FUNCTION__) " called!"); } while(0)
#endif

#if defined(_WIN32)
#  define INTRIN_INLINE __forceinline
#else
#  define INTRIN_INLINE [[using gnu: always_inline]] inline
#endif

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

  StridePtr& operator+=(size_t off)
  {
    m_ptr = (T *)((u8 *)m_ptr + m_stride*off);
    return *this;
  }
  StridePtr& operator-=(size_t off)
  {
    m_ptr = (T *)((u8 *)m_ptr - m_stride*off);
    return *this;
  }

  T *get() const { return m_ptr; }

  size_t stride() const { return m_stride; }

private:
  T *m_ptr;
  size_t m_stride;
};
