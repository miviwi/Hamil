#pragma once

#include <hm/hamil.h>

namespace hm {

// Forward declarations
class PrototypeChunkHandle;
class EntityPrototype;
class EntityPrototypeCache;

namespace detail {
struct CacheEntry;
}
// --------------------

class CachedPrototype {
public:

  const EntityPrototype& prototype() const;

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
  PrototypeChunkHandle chunkByIndex(size_t idx);

  PrototypeChunkHandle allocChunk();

private:
  friend EntityPrototypeCache;

  CachedPrototype() = default;  // These can't be instanitiated directly

  // Creates a CachedPrototype from the given CacheEntry
  //   -  For internal use only (in EntityPrototypeCache)
  static CachedPrototype from_cache_line(detail::CacheEntry *line);

  detail::CacheEntry *m_cached = nullptr;
};

}
