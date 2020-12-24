#include <hm/cachedprototype.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/prototypechunk.h>

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
  assert(m_cached);   // Sanity check

  // Allocate space for a new PrototypeChunkHeader...
  auto chunk_idx = m_cached->headers.emplace();

  //  ...and grab a reference to it
  auto header = &m_cached->headers.back();

  // Do the same for the PrototypeChunk itself...
  auto chunk = UnknownPrototypeChunk::alloc();
  m_cached->chunks.emplace(chunk);

  const auto& proto = m_cached->proto;

  auto chunk_capacity = proto.chunkCapacity();

  header->base_offset = chunk_idx * chunk_capacity;
  header->capacity    = chunk_capacity;

  header->num_entities = 0;

  // TODO: not needed (?)
  header->prototype_cache_id = proto.hash();

  return PrototypeChunkHandle::from_header_and_chunk(*header, chunk);
}

}
