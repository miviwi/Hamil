#pragma once

#include <hm/prototype.h>
#include <hm/prototypechunk.h>
#include <hm/chunkhandle.h>

#include <util/hashindex.h>
#include <util/smallvector.h>
#include <util/passkey.h>

#include <memory>
#include <vector>
#include <optional>
#include <functional>

namespace hm {

// Forward declarations
class PrototypeChunkHandle;
class CachedPrototype;
class EntityQuery;

namespace detail {

struct CacheEntry {
  EntityPrototype proto;

  // Used to uniquely (in the owning EntityPrototypeCache's scope)
  //   identify CachedPrototypes with matching EntityPrototypes
  //  - Can be passed to EntityPrototypeCache::protoByCacheId()
  //    to retrieve a CachedPrototype instead of doing a probe()
  //    with an EntityPrototype
  u32 cache_id;

  using Chunk = UnknownPrototypeChunk;

  util::SmallVector<PrototypeChunkHeader, 64 - sizeof(EntityPrototype)> headers;
  util::SmallVector<Chunk *, 64 - sizeof(u32)> chunks;

  size_t numChunks() const;

  PrototypeChunkHandle chunkAt(size_t idx) const;
};

struct CachePage {
  using Ptr = std::unique_ptr<CachePage>;
  using Entry = CacheEntry;

  enum : size_t {
    PageSize = 1 * 1024,       // 1KiB

    // Keep this a power-of-2, so some mod arithmetic
    //   can be optimized into shifts later in the code
    NumEntriesPerPage = PageSize/sizeof(Entry),
  };

  // See comment above 'NumEntriesPerPage'
  static_assert((NumEntriesPerPage & (NumEntriesPerPage-1)) == 0);

  Entry protos[NumEntriesPerPage];
};

}

class EntityPrototypeCache {
public:
  using ComponentTypeMap = EntityPrototype::ComponentTypeMap;

  enum : u32 { ProtoCacheIdInvalid = ~0u };

  struct PrototypeDesc {
    u32 cache_id = ProtoCacheIdInvalid;     // See CacheEntry::cache_id
    EntityPrototype prototype;

    size_t numChunks() const;
    size_t numEntities() const;

    using ProtoChunkIterFn = std::function<
        void(PrototypeChunkHandle, u32 /* base_offset */, u32 /* num_entities */)
    >;
    const PrototypeDesc& foreachChunkOfPrototype(ProtoChunkIterFn&& fn) const;

  private:
    friend EntityPrototypeCache;

    const detail::CacheEntry *_cache_ref = nullptr;
  };

  EntityPrototypeCache();
  EntityPrototypeCache(const EntityPrototypeCache& other) = delete;

  // Returns a handle to a cache line's CachedPrototype if
  //       fill(a_prototype),   where  =>  proto == a_prototype
  //   has previously been called on the EntityPrototypeCache,
  //   or a std::nullopt otherwise
  std::optional<CachedPrototype> probe(const EntityPrototype& proto);

  // Allocates a cache line for 'proto' and returns a handle
  //   to it which will also be returned by future calls to
  //         probe(a_prototype),    for a_prototype == proto
  //  - Calling fill() for a given 'proto' multiple times
  //    is forbidden and could result in UB
  CachedPrototype fill(const EntityPrototype& proto);

  // - Calling this method with an invalid 'proto_cache_id' is
  //   forbidden and results in UB
  CachedPrototype protoByCacheId(u32 proto_cache_id);

  void dbg_PrintPrototypeCacheStats() const;

//semi-protected:

  using EntityQueryKey = util::PasskeyFor<EntityQuery>;

  using CachedProtoIterFn = std::function<void(const PrototypeDesc&)>;
  const EntityPrototypeCache& foreachCachedProto(CachedProtoIterFn&& fn, EntityQueryKey = {}) const;

private:
  using Entry = detail::CacheEntry;

  static constexpr size_t InitialReservedPages = 128;

  Entry *protoByIndex(size_t idx);
  const Entry *protoByIndex(size_t idx) const;

  std::vector<detail::CachePage> m_pages;
  size_t m_num_protos = 0;

  util::HashIndex m_protos_hash;
};

}
