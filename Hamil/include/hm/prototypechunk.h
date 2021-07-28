#pragma once

#include <hm/hamil.h>
#include <hm/cachedprototype.h>
#include <hm/componentmeta.h>

#include <util/passkey.h>
#include <util/lambdatraits.h>

#include <tuple>
#include <array>
#include <optional>
#include <utility>
#include <type_traits>

namespace hm {

// Forward declarations
class Component;
class ChunkManager;
class EntityPrototypeCache;
class EntityQuery;

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

  template <typename T = u8>
  T *arrayAtOffset(size_t off /* in bytes into the chunk's data */)
  {
    // assert(off+sizeof(T) <= PrototypeChunkSize);

    return (T *)(m_data + off);
  }

  template <typename T = u8>
  const T *arrayAtOffset(size_t off /* in bytes into the chunk's data */) const
  {
    // assert(off+sizeof(T) <= PrototypeChunkSize);

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

class BoundPrototypeChunk {
public:
  BoundPrototypeChunk(const BoundPrototypeChunk&) = delete;
  BoundPrototypeChunk(BoundPrototypeChunk&& other) noexcept;

  BoundPrototypeChunk& operator=(BoundPrototypeChunk&& other) noexcept;

  // Returns the offset (in bytes) into the chunk at which
  //   the data array for 'component' begins
  size_t componentDataArrayOffsetFor(ComponentProtoId component) const;

  // Returns the distance (in bytes) between successive
  //   elements in 'component' data array
  size_t componentDataArrayElemStride(ComponentProtoId component) const;

  template <typename ComponentType>
  ComponentType *componentDataArrayBegin()
  {
    const IComponentMetaclass& component_meta = *metaclass_from_type<ComponentType>();

    return (ComponentType *)componentDataBegin(component_meta.staticData().protoid);
  }

  template <typename ComponentType>
  ComponentType *componentDataArrayEnd()
  {
    const IComponentMetaclass& component_meta = *metaclass_from_type<ComponentType>();

    return (ComponentType *)componentDataEnd(component_meta.staticData().protoid);
  }

  // Fn must have a signature compatible with:
  //    - void(ComponentType&, size_t entity_idx, size_t chunk_idx)
  //  'compatible' meaning the types must match, but
  //  arity isn't imposed (unary+)
  template <
      typename ComponentType, typename Fn,
      typename FnTraits = util::LambdaTraits<Fn>, size_t FnNumArgs = FnTraits::NumArguments,
      typename _ = std::enable_if_t<(FnNumArgs > 0) && (FnNumArgs < 4)>>
  BoundPrototypeChunk& componentDataForeach(Fn&& fn)
  {
    using Fn1stArgType = std::remove_reference_t<
      std::remove_cv_t<typename FnTraits::Arg1stType>>;

    static_assert(std::is_same_v<Fn1stArgType, ComponentType>);

    const auto begin = componentDataArrayBegin<ComponentType>(),
               end   = componentDataArrayEnd<ComponentType>();

    auto it = begin;
    while(it < end) {
      auto& component = *it;

      auto entity_idx = std::distance(begin, it);
      auto chunk_idx  = m_proto_chunk_idx;

      if constexpr (FnNumArgs == 1) {
        fn(component);
      } else if(FnNumArgs == 2) {
        fn(component, entity_idx);
      } else if(FnNumArgs == 3) {
        fn(component, entity_idx, chunk_idx);
      } else {
        static_assert(FnNumArgs <= 2, "TODO: unimplemented 'Fn' call...");
      }

      it++;
    }

    return *this;
  }

  void swap(BoundPrototypeChunk& other) noexcept;

  void *data();
  const void *data() const;

//semi-protected:
  using EntityQueryKey = util::PasskeyFor<EntityQuery>;

  static BoundPrototypeChunk create_handle(
      const EntityPrototypeCache *proto_cache, PrototypeChunk *chunk, u32 chunk_idx, u32 cacheid,
      EntityQueryKey = {}
  );

protected:
  BoundPrototypeChunk() = default;   // Disallow manual instantiation

private:
  const CachedPrototype& fetchPrototypeRef() const;

  Component *componentDataBegin(ComponentProtoId component);
  Component *componentDataEnd(ComponentProtoId component);

  const EntityPrototypeCache *m_protos_cache = nullptr;
  PrototypeChunk             *m_chunk        = nullptr;

  u32 m_proto_chunk_idx = /* sentinel */ ~0u;

  u32 m_proto_cacheid = /* EntityProttotypeCache::ProtoCacheIdInvalid */ ~0u;

  mutable std::optional<CachedPrototype> m_proto = std::nullopt;    // Lazy-init
};

}

namespace std {

using BoundPrototypeChunk = hm::BoundPrototypeChunk;

template <>
inline void swap<BoundPrototypeChunk>(
    BoundPrototypeChunk &lhs, BoundPrototypeChunk &rhs) noexcept
{
  lhs.swap(rhs);
}

}

