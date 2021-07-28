#pragma once

#include <hm/hamil.h>
#include <hm/prototypechunk.h>

#include <array>
#include <memory>
#include <vector>

namespace hm {

// Forward declarations
class CachedPrototype;
class PrototypeChunk;
class BoundPrototypeChunk;
class IEntityManager;
// --------------------

// 4MiB of PrototypeChunks/page = 4MiB / 16KiB (per chunk) = 256 chunks/page
static constexpr size_t ChunkAllocatorPageSize = (4 * 1024*1024)/PrototypeChunkSize;

using ChunkAllocatorPage = std::array<u8, ChunkAllocatorPageSize>;

using ChunkAllocatorPagePtr = std::unique_ptr<ChunkAllocatorPage>;

class ChunkManager {
public:
  using Ptr = std::unique_ptr<ChunkManager>;

  ChunkManager();

  // Allocate a new PrototypeChunk very fast (except every ChunkAllocatorPageSize
  //   bytes of chunks) and /practically/ always at a sequantial address (except at
  //   page boundaries) as they're taken from pages in a 'slab'
  // TODO:
  //   - write an implementation which actually matches the above description
  //     as currently a regular heap allocation is performed :)
  //   - use a free list (or something else?) so chunks can be reused after
  //     calling freeChunk() on them instead of reclaming only when whole
  //     pages are freed, to waste less memory (and more importantly - reduce
  //     cache pollution)
  UnknownPrototypeChunk *allocChunk();

  // Reclaims 'chunk' but not the underlying pages in RAM so a later call to
  //   allocChunk() will recycle the freed chunk
  //  - 'chunk' MUST'VE come from this ChunkManager or UB will occur, as
  //    this is NOT checked by this method!
  ChunkManager& freeChunk(UnknownPrototypeChunk *chunk);

private:
  // Allocates a fresh ChunkAllocatorPage and stores a pointer to it
  //   at tthe tail of the slab returning the index at which it has
  //   been placed
  size_t acquireNewPage();

  std::vector<ChunkAllocatorPagePtr> m_slab;
};

ChunkManager::Ptr create_chunk_manager();

}
