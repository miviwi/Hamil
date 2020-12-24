#include <hm/prototypechunk.h>
#include <hm/prototype.h>
#include <hm/cachedprototype.h>
#include <hm/chunkhandle.h>

#include <components.h>
#include <hm/components/all.h>

#include <cassert>

namespace hm {

PrototypeChunkHandle PrototypeChunkHandle::from_header_and_chunk(
    const PrototypeChunkHeader& header, UnknownPrototypeChunk *chunk
  )
{
  auto h = PrototypeChunkHandle();

  h.m_header = header;
  h.m_chunk  = chunk;

  return h;
}

UnknownPrototypeChunk *UnknownPrototypeChunk::alloc()
{
  return new PrototypeChunk<GameObject>();
}

}
