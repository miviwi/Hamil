#include <gx/memorypool.h>

#include <cassert>

namespace gx {

MemoryPool::MemoryPool(size_t size) :
  m_pool(new byte[size])
{
  auto ptr = align((uintptr_t)m_pool.get());

  m_ptr = (byte *)ptr;
  m_rover = m_ptr;
  m_end = m_pool.get() + size;
}

MemoryPool::Handle MemoryPool::alloc(size_t sz)
{
  size_t aligned_sz = align(sz);

  if(m_rover+aligned_sz >= m_end) return Invalid;

  auto ptr = (Handle)((m_rover - m_ptr) & HandleMask);
  m_rover += aligned_sz;

  return ptr;
}

void *MemoryPool::ptr(Handle h)
{
  return m_ptr + h;
}

void MemoryPool::purge()
{
  m_rover = m_ptr;
}

uintptr_t MemoryPool::align(uintptr_t ptr)
{
  // Align 'ptr' to AllocAlign-byte boundary
  return (ptr + (AllocAlign-1)) & ~AllocAlignMask;
}

}