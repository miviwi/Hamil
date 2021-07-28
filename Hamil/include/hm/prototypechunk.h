#pragma once

#include <hm/hamil.h>

#include <tuple>
#include <array>
#include <optional>
#include <type_traits>

namespace hm {

// Forward declarations
class ChunkManager;

class UnknownPrototypeChunk;
// --------------------

static constexpr size_t PrototypeChunkSize = 16 * 1024;   // 16KiB = 4 pages

// Used to type-erase instantiations of PrototypeChunk<...> template,
//   which allows storing and passing around PrototypeChunk */& in
//   non-templated code with better safety than void* for example
class alignas(PrototypeChunkSize) UnknownPrototypeChunk {
public:

//protected:
  UnknownPrototypeChunk() = default;    // Force using a derived class

  template <typename T>
  T& getDataRef() { return *(T *)m_data; }

  template <typename T>
  const T& getDataRef() const { return *(const T *)m_data; }

  template <typename T>
  T *arrayAtOffset(size_t off /* in bytes into the chunk's data */)
  {
    assert(off+sizeof(T) <= PrototypeChunkSize);

    return (T *)(m_data + off);
  }

  template <typename T>
  const T *arrayAtOffset(size_t off /* in bytes into the chunk's data */) const
  {
    assert(off+sizeof(T) <= PrototypeChunkSize);

    return (const T *)(m_data + off);
  }

private:
  u8 m_data[PrototypeChunkSize];
};

// XXX: here lay the chunk with compile-time known layout...
class PrototypeChunk : public UnknownPrototypeChunk {
public:
  using UnknownPrototypeChunk::UnknownPrototypeChunk;
};

}
