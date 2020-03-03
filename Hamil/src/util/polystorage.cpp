#include <util/polystorage.h>

#include <cstdlib>

namespace polystorage_detail {

void *Allocator::alloc(size_t sz)
{
  return ::malloc(sz);
}

void Allocator::free(void *ptr, size_t sz)
{
  ::free(ptr);
}

}
