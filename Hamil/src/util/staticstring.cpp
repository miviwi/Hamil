#include <util/staticstring.h>
#include <util/hash.h>

namespace util {

size_t StaticString::hash() const
{
  return xxh::xxhash<64>(m_str.data(), m_str.size());
}

}
