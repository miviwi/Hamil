#include <hm/cachedprototype.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/prototypechunk.h>

#include <components.h>

#include <limits>

#include <cassert>

namespace hm {

using detail::CacheEntry;

CachedPrototype CachedPrototype::from_cache_line(CacheEntry *cache_line)
{
  assert(cache_line && "CachedPrototype::from_cache_line() cache_line==NULL!");

  auto proto = CachedPrototype();

  proto.m_cached = cache_line;

  return proto;
}

size_t CachedPrototype::numChunks() const
{
  assert(m_cached);

  return m_cached->numChunks();
}

const EntityPrototype& CachedPrototype::prototype() const
{
  assert(m_cached);

  return m_cached->proto;
}

u32 CachedPrototype::cacheId() const
{
  assert(m_cached);

  return m_cached->cache_id;
}

size_t CachedPrototype::numEntities() const
{
  if(m_cached->chunks.empty()) return 0;  // Cleaner code with early-out :)

  auto num_chunks = numChunks();

  // We'll count slightly more optimally than the naive
  //    foreach(chunk) num_entities += chunk.num_entities
  //  thanks to the invariant that entities in a chunk
  //  are always tightly packed
  auto last_chunk = m_cached->chunkAt(num_chunks-1);

  // Same for all 'm_cached->chunks' as they all have
  //   the same underlying type
  auto chunk_capacity = last_chunk.capacity();

  // Last chunk could have null entities at the end
  //   so account for that below
  auto tail_len = last_chunk.numEntities();

  size_t num_entities = chunk_capacity * (/* full chunks */ num_chunks-1) 
    + tail_len /* account for the tail... */;

  return num_entities;
}

PrototypeChunkHandle CachedPrototype::chunkByIndex(size_t idx)
{
  assert(idx < numChunks() && "chunk 'idx' out of range! (> numChunks())");
  assert(idx < std::numeric_limits<u32>::max()); // Sanity check ;)

  const auto trunc_idx = (u32)idx; // safety (no truncation) assert()'ed above

  const auto& header = m_cached->headers.at(trunc_idx);
  auto chunk_ptr     = m_cached->chunks.at(trunc_idx);

  return PrototypeChunkHandle::from_header_and_chunk(header, chunk_ptr);
}

PrototypeChunkHandle CachedPrototype::allocChunk()
{
  assert(m_cached &&                                         // Sanity check
      m_cached->headers.size() == m_cached->chunks.size());

  // Allocate space for a new PrototypeChunkHeader...
  auto chunk_idx = m_cached->headers.emplace();

  //  ...and grab a reference to it
  auto header = &m_cached->headers.back();

  // Do the same for the PrototypeChunk itself...
  auto chunk = UnknownPrototypeChunk::alloc();
  m_cached->chunks.emplace(chunk);

  const auto& proto = m_cached->proto;

  auto chunk_capacity = proto.chunkCapacity();

#if !defined(NDEBUG)
  header->dbg_proto = &proto;
#endif

  header->base_offset = chunk_idx * chunk_capacity;
  header->capacity    = chunk_capacity;

  header->num_entities = 0;

  return PrototypeChunkHandle::from_header_and_chunk(*header, chunk);
}

Component *CachedPrototype::chunkComponentDataByIndex(
    ComponentProtoId component, size_t idx
  )
{
  assert(m_cached);

  const auto& proto = m_cached->proto;
  assert(proto.includes(component) &&
      "data for requested Component not included in the EntityPrototype!");

  assert(idx < std::numeric_limits<u32>::max() && idx < m_cached->chunks.size());

  auto chunk = m_cached->chunks.at((u32)idx);
  const auto chunk_offset = proto.componentDataOffsetInSoAEntityChunk(component);

  return chunk->arrayAtOffset<Component>(chunk_offset);
}

u32 CachedPrototype::allocEntity()
{
  auto wtail_chunk_idx = numChunks() - 1;   // Allocate at the last chunk - the 'tail'
  assert(wtail_chunk_idx < std::numeric_limits<u32>::max());

  auto tail_idx = (u32)wtail_chunk_idx;  // assert() ensures no truncation...

  assert(m_cached->headers.size() == m_cached->chunks.size()); // Sanity check

  auto& tail_header = m_cached->headers.at(tail_idx);
  //auto tail_chunk   = m_cached->chunks.at(tail_idx);

  // No more space - need to allocChunk() to make room for the entity beforehand
  if(tail_header.num_entities >= tail_header.capacity) return AllocEntityInvalidId;

  // Place the new entity at the back of the chunk...
  u32 entity_id = tail_header.base_offset + tail_header.num_entities;

  tail_header.num_entities++;     // And increment their count in this chunk

  return entity_id;
}

PrototypeChunkHandle CachedPrototype::chunkForEntityId(u32 entity_id)
{
  auto chunk_idx = chunkIdxForEntityId(entity_id);

  return PrototypeChunkHandle::from_header_and_chunk(
      m_cached->headers.at(chunk_idx), m_cached->chunks.at(chunk_idx)
  );
}

Component *CachedPrototype::componentDataForEntityId(
    ComponentProtoId component, u32 id
  )
{
  auto mc = metaclass_from_protoid(component);

  auto chunk_idx = chunkIdxForEntityId(id);
  const auto& header = m_cached->headers.at(chunk_idx);
  auto chunk = m_cached->chunks.at(chunk_idx);

  assert(m_cached);

  const auto& proto = m_cached->proto;
  assert(proto.includes(component) &&
      "data for requested Component not included in the EntityPrototype!");

  const auto entity_index_in_chunk = id - header.base_offset;

  const auto data_stride = mc->staticData().data_size;

  const auto chunk_offset = proto.componentDataOffsetInSoAEntityChunk(component);
  const auto entity_offset = chunk_offset + entity_index_in_chunk*data_stride;

  const auto component_data = chunk->arrayAtOffset<Component>(entity_offset);

  return component_data;
}

u32 CachedPrototype::chunkIdxForEntityId(u32 entity_id)
{
  assert(entity_id < numEntities() &&
      "'entity_id' for CachedPrototype::chunkForEntityId() out of range!");

  // Calling this method when m_cached->headers.empty(), is forbidden
  //   (and asserted above), thus dereferencing front() will always give
  //   valid data
  auto chunk_capacity = m_cached->headers.front().capacity;

  // Sanity check - the capacity should be consistent accross all
  //   chunks of the same EntityPrototype
  assert(chunk_capacity == m_cached->proto.chunkCapacity());

  auto chunk_idx = entity_id / chunk_capacity;

  assert(chunk_idx < m_cached->headers.size() &&             // Sanity check
      m_cached->headers.size() == m_cached->chunks.size());

  return chunk_idx;
}

}
