#pragma once

#include <hm/hamil.h>

namespace hm {

// Forward declarations
class Component;
class PrototypeChunkHandle;
class EntityPrototype;
class EntityPrototypeCache;
class ChunkManager;

namespace detail {
struct CacheEntry;
}
// --------------------

// TODO: rename hm::EntityPrototype -> hm::EntityPrototypeDesc,
//    along with hm::CachedPrototype -> hm::EntityPrototype,
//  which better reflects their function (see comment in
//  hm/prototype.h for further thougths..)
class CachedPrototype {
public:
  enum : u32 { AllocEntityInvalidId = ~0u };

  const EntityPrototype& prototype() const;

  // Returns a value which is guaranteed to uniquely identify
  //   a CachedPrototype for an EntityPrototype in the scope of
  //   an EntityPrototypeCache
  //  - Can be passed to EntityPrototypeCache::protoByCacheId()
  //    to retrieve this CachedPrototype
  u32 cacheId() const;

  // Returns the number of PrototypeChunks allocated
  //   for this prototype
  //  - TODO: what happens when a chunk gets emptied?
  //       > do we release it right away..?
  //       > keep it..? (is it included in this count then?)
  size_t numChunks() const;

  // Returns the sum of the number of entities stored accross
  //   all of this prototype's PrototypeChunks
  size_t numEntities() const;

  //  - 'idx' MUST be < numChunks()!
  PrototypeChunkHandle chunkByIndex(size_t idx) const;

  Entity entityIdByAllocId(u32 alloc_id) const;

  PrototypeChunkHandle allocChunk(ChunkManager *chunk_man);

  // Returns a pointer to the array of Component data denoted by 'component'
  //   stored in the chunk retrieved by chunkByIndex(idx) downcast to Component[]
  //  - 'component' MUST be include()'d in the source EntityPrototype
  Component *chunkComponentDataByIndex(ComponentProtoId component, size_t idx);

  // Allocates a new entity with this prototype and returns an
  //   id (which would be just an index if Entities were stored
  //   in flat arrays) which can be used to reffer to it in the
  //   context of this CachedPrototype
  //  - If 'AllocEntityInvalidId' is returned the allocation failed
  //    for some reason (i.e. AllocEntityInvalidId is NOT a valid
  //    CachedPrototype entity id).
  //    Most likely the tail chunk's capacity (and thus the capacity
  //    of all the allocated chunks, as the Component data in a chunk
  //    will never have any holes created by ex. removing an Entity
  //    as it's continually defragemented/kept tighly packed) ran out
  //    and a new chunk must be allocated via allocChunk()
  // XXX: should a new chunk be allocated if all are full() instead
  //    of failing (returning AllocEntityInvalidId)?
  u32 allocEntity();

  // Returns a handle to the chunk in which the entity with allocation-id
  //   'entity_id' is stored, in particular stored under the index =>
  //           entity_id - chunk.entityBaseIndex()
  //    where 'chunk' is the handle returned by this method
  PrototypeChunkHandle chunkForEntityAllocId(u32 alloc_id);

  // Returns a pointer to data for Component denoted by 'component'
  //   owned by entity under allocation-id 'id'
  Component *componentDataForEntityId(ComponentProtoId component, u32 id);

private:
  friend EntityPrototypeCache;

  CachedPrototype() = default;  // These can't be instantiated directly

  // Creates a CachedPrototype from the given CacheEntry
  //   -  For internal use only (in EntityPrototypeCache)
  static CachedPrototype from_cache_line(detail::CacheEntry *line);

  u32 chunkIdxForEntityId(u32 entity_id);

  detail::CacheEntry *m_cached = nullptr;
};

}
