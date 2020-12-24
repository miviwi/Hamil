#pragma once

#include <hm/hamil.h>

namespace hm {

// Forward declarations
class EntityPrototype;
class UnknownPrototypeChunk;

// Logically this data should be stored along with
//   PrototypeChunk<Component...> component data for
//   a chunk, however, it is a separate struct to
//   allow an AoS type layout for the chunks (i.e.
//   in another array beside the one with actual data
//   instead of  intreleaved)
//  TODO: does this boost perf? (or at leat not harm it)
struct PrototypeChunkHeader {
#if !defined(NDEBUG)
  const EntityPrototype *dbg_proto;
#endif

  u32 base_offset;   // Global offset of this chunk's first entity,
                     //   calculated as if all the entities with
                     //   the same type are stored sequentially 
                     //   in a single array

  u32 capacity;   // Stores the number of entities which this
                  //   concrete PrototypeChunk<Components...>
                  //   can store:
                  //       PrototypeChunk<...>::NumComponentsPerType

  u32 num_entities;  // Number of entities currently stored in this chunk

};

// Handle to a single PrototypeChunk's data along with it's associated
//   'PrototypeChunkHeader' (which isn't stored interleaved among
//   related PrototypeChunk's data but instead next to it, i.e. in a
//   SoA layout)
//  - These are to be retrieved from 'EntityPrototypeCache' object(s)
//    via probe()/fill() ONLY
class PrototypeChunkHandle {
public:
  static PrototypeChunkHandle from_header_and_chunk(
      const PrototypeChunkHeader& header, UnknownPrototypeChunk *chunk
  );

  // Returns the base index for this chunk's entities
  //   - See PrototypeChunkHeader::base_offset description above
  u32 entityBaseIndex() const { return m_header.base_offset; }

  // Returns the maximum number of entities which can be stored
  //   in PrototypeChunks of this underlying type (given a concrete
  //   PrototypeChunk<Components...> and another with matching
  //   'Components...' this value will always be equal)
  size_t capacity() const { return m_header.capacity; }

  // Returns the number of entities currently stored in this PrototypeChunk
  size_t numEntities() const { return m_header.num_entities; }

  // Returns 'true' when all available entity slots of this chunk
  //   have been filled
  bool full() const { return m_header.num_entities >= m_header.capacity; }

  // Returns 'true' if all of the chunk's capacity() is vacant
  bool empty() const { return m_header.num_entities == 0; }

  // Returns 'true' when this chunk is purgeable, that is it's memory can
  //   be freed/reused without any perceeding operations.
  //  - Chunks are considered 'purgeable' if they are empty() and there is
  //    at least one more with the same EntityPrototype (the first chunk
  //    for a given prototype is always kept around)
  bool purgeable() const { return m_header.base_offset > 0 && empty(); }

  void dbg_PrintChunkStats() const;

private:
  PrototypeChunkHandle() = default;   // Disallow direct instantiation

  PrototypeChunkHeader m_header;
  UnknownPrototypeChunk *m_chunk = nullptr;
};

}
