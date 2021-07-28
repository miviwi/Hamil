#include <hm/prototypechunk.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/cachedprototype.h>
#include <hm/chunkhandle.h>
#include <hm/component.h>

#include <components.h>

#include <utility>

#include <cassert>

namespace hm {

BoundPrototypeChunk BoundPrototypeChunk::create_handle(
    const EntityPrototypeCache *proto_cache, PrototypeChunk *chunk, u32 chunk_idx, u32 proto_cacheid,
    EntityQueryKey)
{
  assert(proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid);

  BoundPrototypeChunk h;
  h.m_protos_cache    = proto_cache;
  h.m_chunk           = chunk;
  h.m_proto_chunk_idx = chunk_idx;
  h.m_proto_cacheid   = proto_cacheid;

  return h;
}

BoundPrototypeChunk::BoundPrototypeChunk(BoundPrototypeChunk&& other) noexcept :
    BoundPrototypeChunk()
{
  std::swap(*this, other);
}

BoundPrototypeChunk& BoundPrototypeChunk::operator=(BoundPrototypeChunk&& other) noexcept
{
  std::swap(*this, other);
  return *this;
}

// TODO: maybe think about doing some memoization or LRU cache for offsets?
size_t BoundPrototypeChunk::componentDataArrayOffsetFor(ComponentProtoId component) const
{
  assert(
         m_protos_cache && m_chunk && m_proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid
      && "componentArrayDataOffsetFor() called on a NULL BoundPrototypeChunk handle!");

  const auto &proto = fetchPrototypeRef(); // We'll need to lookup some data from 'm_proto'

  assert(proto.prototype().includes(component) && "'component' isn't present in this chunk!");

  return proto.prototype()
      .componentDataOffsetInSoAEntityChunk(component);
}

size_t BoundPrototypeChunk::componentDataArrayElemStride(ComponentProtoId component) const
{
  assert(
         m_protos_cache && m_chunk && m_proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid
      && "componentArrayDataOffsetFor() called on a NULL BoundPrototypeChunk handle!");

  // Sanity check
  const auto &proto = fetchPrototypeRef();
  assert(proto.prototype().includes(component) && "'component' isn't present in this chunk!");

  const auto &component_meta = metaclass_from_protoid(component)
                                   ->staticData();

  // XXX: all component_meta.flags must be taken into account (?)

  return component_meta.data_size;
}

const CachedPrototype& BoundPrototypeChunk::fetchPrototypeRef() const
{
  assert(m_proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid && m_protos_cache);

  /*
   *  if(m_proto.has_value()) return;    // XXX: will this happen so often to take into account?
   */

  auto proto = m_protos_cache->protoByCacheId(m_proto_cacheid);
  const auto &proto_ref = m_proto.emplace(proto);

  return proto_ref;
}

Component *BoundPrototypeChunk::componentDataBegin(ComponentProtoId component)
{
  assert(m_protos_cache && m_chunk && m_proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid);

  const auto &proto = fetchPrototypeRef();
  assert(proto.prototype().includes(component)); // Guard metthod from returning garbage...

  auto offset = proto.prototype()
                    .componentDataOffsetInSoAEntityChunk(component);

  return m_chunk->arrayAtOffset<Component>(offset);
}

Component *BoundPrototypeChunk::componentDataEnd(ComponentProtoId component)
{
  assert(m_protos_cache && m_chunk && m_proto_cacheid != EntityPrototypeCache::ProtoCacheIdInvalid);

  const auto &proto = fetchPrototypeRef();
  assert(proto.prototype().includes(component)); // Guard metthod from returning garbage...

  auto offset = proto.prototype()
                    .componentDataOffsetInSoAEntityChunk(component);

  auto num_array_entities = proto.numEntities();
  auto array_elem_stride = componentDataArrayElemStride(component);

  return m_chunk->arrayAtOffset<Component>(offset + num_array_entities*array_elem_stride);
}

void BoundPrototypeChunk::swap(BoundPrototypeChunk& other) noexcept
{
  std::swap(m_protos_cache, other.m_protos_cache);
  std::swap(m_chunk, other.m_chunk);
  std::swap(m_proto_cacheid, other.m_proto_cacheid);
}

void *BoundPrototypeChunk::data()
{
  assert(m_chunk);
  return m_chunk->arrayAtOffset(0);
}

const void *BoundPrototypeChunk::data() const
{
  assert(m_chunk);
  return m_chunk->arrayAtOffset(0);
}

}
