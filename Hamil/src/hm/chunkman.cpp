#include <hm/chunkman.h>
#include <hm/chunkhandle.h>
#include <hm/prototype.h>
#include <hm/prototypechunk.h>
#include <hm/cachedprototype.h>
#include <hm/entityman.h>

#include <cassert>

namespace hm {

ChunkManager::ChunkManager()
{
}

UnknownPrototypeChunk *ChunkManager::allocChunk()
{
  return new PrototypeChunk();

  STUB();

  return nullptr;
}

ChunkManager& ChunkManager::freeChunk(UnknownPrototypeChunk *chunk)
{
  delete (PrototypeChunk *)chunk;

  return *this;

  STUB();

  return *this;
}

size_t ChunkManager::acquireNewPage()
{
  auto index = m_slab.size();

  auto page = new ChunkAllocatorPage();
  m_slab.emplace_back(page);

  return index;
}

ChunkManager::Ptr create_chunk_manager()
{
  return ChunkManager::Ptr(new ChunkManager());
}

}
