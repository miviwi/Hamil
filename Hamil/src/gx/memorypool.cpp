#include <gx/memorypool.h>

#include <cassert>

namespace gx {

MemoryPool::MemoryPool(size_t size) :
  m_pool(new byte[size])
{
  auto ptr = align((uintptr_t)m_pool.get());

  m_rover = (byte *)ptr;
  m_end = m_pool.get() + size;
}

MemoryPool::Handle MemoryPool::alloc(size_t sz)
{
  if(m_rover >= m_end) return Invalid;

  auto ptr = (Handle)((m_rover - m_pool.get()) & HandleMask);
  m_rover += align(sz);

  return ptr;
}

void *MemoryPool::ptr(Handle h)
{
  return m_pool.get() + h;
}

void MemoryPool::purge()
{
  m_rover = (byte *)align((uintptr_t)m_pool.get());
}

uintptr_t MemoryPool::align(uintptr_t ptr)
{
  // Align 'ptr' to AllocAlign-byte boundary
  return (ptr + (AllocAlign-1)) & ~AllocAlignMask;
}

}