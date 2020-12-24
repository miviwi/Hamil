#include <hm/prototypechunk.h>
#include <hm/prototype.h>
#include <hm/cachedprototype.h>
#include <hm/chunkhandle.h>

#include <components.h>
#include <hm/components/all.h>

#include <cassert>

namespace hm {

UnknownPrototypeChunk *UnknownPrototypeChunk::alloc()
{
  return new PrototypeChunk<GameObject>();
}

}
