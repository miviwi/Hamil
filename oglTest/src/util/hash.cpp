#include <util/hash.h>

namespace util {

size_t hash(const void *data, size_t size)
{
  return xxh::xxhash<64>(data, size);
}

}