#include <hm/prototypecache.h>
#include <hm/prototypechunk.h>
#include <hm/cachedprototype.h>
#include <hm/chunkhandle.h>

#include <util/fixedbitvector.h>

#include <cassert>

namespace hm {

EntityPrototypeCache::EntityPrototypeCache() :
  m_protos_hash(1024, 1024)
{
  m_pages.reserve(InitialReservedPages);

  // Allocate a page ahead of time as in most cases we'll need at least one
  m_pages.emplace_back();
}

auto EntityPrototypeCache::probe(const EntityPrototype& proto) ->
    std::optional<CachedPrototype>
{
  const auto index = m_protos_hash.find(proto.hash(),
      [this,&proto](auto key, auto index) -> bool {
        const auto entry = protoByIndex(index);
        if(!entry) return false;

        return entry->proto.equal(proto);
      }
  );

  if(index == util::HashIndex::Invalid) return std::nullopt;

  // Calculate the appropriate index into 'm_pages', then
  //   into the CachePage itself and fetch a pointer to
  //   the desired CacheEntry
  auto cache_line = protoByIndex(index);

  return CachedPrototype::from_cache_line(cache_line);
}

auto EntityPrototypeCache::fill(const EntityPrototype& proto) ->
    CachedPrototype
{
  assert(!probe(proto) &&
      "EntityPrototypeCache::fill() called with the same 'proto' twice!");

  using Page = detail::CachePage;

  if(m_num_protos+1 >= m_pages.size()*Page::NumEntriesPerPage) {
    // Have to allocate a new detail::CachePage...
    m_pages.emplace_back();
  }

  auto& page = m_pages.back();

  const auto off_in_page = m_num_protos % Page::NumEntriesPerPage;
  auto cache_line = &page.protos[off_in_page];

  // Store the new entry's index into the HashIndex
  const auto cache_line_idx = m_num_protos;

  m_num_protos++;     //  ...and move rover forward

  // Initialize the CacheEntry...
  cache_line->proto = proto;
  cache_line->cache_id = cache_line_idx;

  m_protos_hash.add(proto.hash(), cache_line_idx);

  return CachedPrototype::from_cache_line(cache_line);
}

CachedPrototype EntityPrototypeCache::protoByCacheId(u32 cache_id)
{
  auto proto = protoByIndex(cache_id);

  assert(proto && "protoByCacheId() given invalid 'proto_cache_id'!");

  return CachedPrototype::from_cache_line(proto);
}

const EntityPrototypeCache& EntityPrototypeCache::foreachCachedProto(
      CachedProtoIterFn&& fn, EntityQueryKey
  ) const
{
  using namespace detail;

  static constexpr size_t NumPageEntries = CachePage::NumEntriesPerPage;

  for(const auto& cache_page : m_pages) {
    for(size_t entry_idx = 0; entry_idx < NumPageEntries; entry_idx++) {
      const auto& entry = cache_page.protos[entry_idx];

      PrototypeDesc proto_desc;
      proto_desc.cache_id   = entry.cache_id;
      proto_desc.prototype  = entry.proto;
      proto_desc._cache_ref = &entry;

      fn(proto_desc);
    }
  }

  return *this;
}

auto EntityPrototypeCache::protoByIndex(size_t idx) ->
    Entry *
{
  if(idx >= m_num_protos) return nullptr;     // idx out of range

  const auto page_idx = idx / detail::CachePage::NumEntriesPerPage,
        inpage_idx = idx % detail::CachePage::NumEntriesPerPage;

  auto& page = m_pages[page_idx];

  return std::addressof(page.protos[inpage_idx]);
}

auto EntityPrototypeCache::protoByIndex(size_t idx) const ->
    const Entry *
{
  // Ugly, but reduces duplicated code
  return const_cast<EntityPrototypeCache *>(this)->protoByIndex(idx);
}

void EntityPrototypeCache::dbg_PrintPrototypeCacheStats() const
{
#if !defined(NDEBUG)
  auto print_proto = [](const detail::CacheEntry& entry) {
    printf("EntityPrototype[%s] { .cache_id=0x%.8x, .numChunks=%zu } =>\n",
        util::to_str(entry.proto.components()).data(), entry.cache_id,
        entry.numChunks()
    );

    entry.proto.dbg_PrintComponents();

    printf("\n");
  };

  printf("EntityPrototypeCache@%p { m_pages.size()=%zu, m_num_protos=%zu } =>\n",
      this,
      m_pages.size(), m_num_protos
  );

  for(size_t p = 0; p < m_pages.size()-1; p++) {
    const auto& page = m_pages.at(p);

    for(size_t i = 0; i < detail::CachePage::NumEntriesPerPage; i++) {
      print_proto(page.protos[i]);
    }
  }

  for(size_t i = 0; i < m_num_protos%detail::CachePage::NumEntriesPerPage; i++) {
    print_proto(m_pages.back().protos[i]);
  }
#endif
}

using PrototypeDesc = EntityPrototypeCache::PrototypeDesc;
size_t PrototypeDesc::numChunks() const
{
  return _cache_ref->numChunks();
}

size_t PrototypeDesc::numEntities() const
{
  using namespace detail;

  // We rely on entities being tightly packed in the chunks,
  //  a guaranteed property of the chunk cache
  auto num_chunks = numChunks();
  const auto& tail_chunk = _cache_ref->chunkAt(num_chunks-1);

  return (num_chunks-1)*CachePage::NumEntriesPerPage + tail_chunk.numEntities();
}

auto PrototypeDesc::foreachChunkOfPrototype(ProtoChunkIterFn&& fn) const
    -> const PrototypeDesc&
{
  const size_t num_chunks = _cache_ref->numChunks();
  for(size_t chunk_idx = 0; chunk_idx < num_chunks; chunk_idx++) {
    const auto& header = _cache_ref->headers.at(chunk_idx);

    u32 base_offset  = header.base_offset,
        num_entities = header.num_entities;

    fn(_cache_ref->chunkAt(chunk_idx), base_offset, num_entities);
  }

  return *this;
}

}

namespace hm::detail {

size_t CacheEntry::numChunks() const
{
  assert(headers.size() == chunks.size());   // Sanity check

  return headers.size();
}

PrototypeChunkHandle CacheEntry::chunkAt(size_t idx) const
{
  assert(idx < headers.size() && "CacheEntry::chunkAt() idx out of bounds!");

  assert(headers.size() ==  chunks.size());   // Sanity check

  const auto trunc_idx = (u32)idx; // safety (no truncation) assert()'ed
                                   //   above => util::SmallVector::size()
                                   //   returns value < 2^32

  auto& header = headers.at(trunc_idx);
  auto chunk   = chunks.at(trunc_idx);

  return PrototypeChunkHandle::from_header_and_chunk(header, chunk);
}

}

