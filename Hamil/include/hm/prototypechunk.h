#pragma once

#include <hm/hamil.h>
#include <hm/component.h>

#include <tuple>
#include <array>
#include <optional>
#include <type_traits>

namespace hm {

// Forward declarations
class UnknownPrototypeChunk;

namespace detail {

template <typename T, size_t N>
struct ChunkComponentStorage {
  static_assert(std::is_base_of_v<Component, T>);

  u8 raw_storage[sizeof(T)*N];

  T& at(size_t idx) { return ((T *)raw_storage)[idx]; }
};

template <size_t NumOfEach, typename... Components>
struct ChunkStorage : public ChunkComponentStorage<Components, NumOfEach>... {

  template <typename T>
  ChunkComponentStorage<T, NumOfEach> *storage()
  {
    return (ChunkComponentStorage<T, NumOfEach> *)this;
  }

};

template <typename... Components>
struct ProtoComponents : Components... {
  static constexpr size_t Size = sizeof(ProtoComponents);
};

template <typename... Components>
static constexpr size_t ProtoComponentsSize = ProtoComponents<Components...>::Size;

}

static constexpr size_t PrototypeChunkSize = 16 * 1024;   // 16KiB = 4 pages

// Used to type-erase instantiations of PrototypeChunk<...> template,
//   which allows storing and passing around PrototypeChunk */& in
//   non-templated code with better safety than void* for example
class alignas(PrototypeChunkSize) UnknownPrototypeChunk {
public:
  // Allocates a PrototypeChunk and returns a pointer to it
  //   - TODO: something other than 'operator new' should propably
  //           be used internally by this method so the chunk's
  //           data will be contiguous in memory, can be easily
  //           reused after free() calls and so said free() calls
  //           can be combined
  static UnknownPrototypeChunk *alloc();

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

// The more though I give it the more I believe that this class
//   doesn't solve any problems as EntityPrototypes must allow
//   run-time promotion/demotion (inclusion of more/less Components)
//   regardless if it can be done at compile time, which will cause
//   duplication of data offset/size calculation logic for chunks in
//   TMP-style code. Can already see the maintenance nightmare :)
// TODO: Throw this out - or..?
template <typename... Components>
class PrototypeChunk : public UnknownPrototypeChunk {
public:

private:
  enum : size_t {
    ProtoComponentsSize = detail::ProtoComponentsSize<Components...>,
    NumComponentsPerType = PrototypeChunkSize/ProtoComponentsSize,
  };

  using Storage = detail::ChunkStorage<NumComponentsPerType, Components...>;

  template <typename T>
  using ComponentStorage = detail::ChunkComponentStorage<T, NumComponentsPerType>;

  Storage& storage() { return getDataRef<Storage>(); }
  const Storage& storage() const { return getDataRef<Storage>(); }
};

}
