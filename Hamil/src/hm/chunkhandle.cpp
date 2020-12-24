#include <hm/chunkhandle.h>
#include <hm/prototype.h>

#include <util/fixedbitvector.h>

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

void PrototypeChunkHandle::dbg_PrintChunkStats() const
{
#if !defined(NDEBUG)
  printf(
      "PrototypeChunk@%p {\n"
      "    .capacity=%zu, .numEntities=%zu, .full? %s\n"
      "    .entityBaseIndex=%u\n"
      "    .proto[%s]\n"
      "}\n",
      m_chunk,
      capacity(), numEntities(), full() ? "yes" : "no",
      entityBaseIndex(),
      util::to_str(m_header.dbg_proto->components()).data()
  );
#endif
}

}
