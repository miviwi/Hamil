#include <gx/memorypool.h>

#include <cassert>

namespace gx {

MemoryPool::MemoryPool(size_t size)
{
  m_pool = (byte *)malloc(size + MaxSizeDefficit);
  checkMalloc();

  auto ptr = align((uintptr_t)m_pool);

  m_ptr = (byte *)ptr;
  m_rover = m_ptr;
  m_end = m_pool + size + MaxSizeDefficit;
}

MemoryPool::~MemoryPool()
{
  free(m_pool);
}

MemoryPool::Handle MemoryPool::alloc(size_t sz)
{
  size_t aligned_sz = align(sz);

  if(m_rover+aligned_sz >= m_end) return Invalid;

  auto ptr = (Handle)((m_rover - m_ptr) & HandleMask);
  m_rover += aligned_sz;

  return ptr;
}

void MemoryPool::grow(size_t sz)
{
  auto current_sz = m_end - m_pool;

  resize(current_sz + sz);
}

// TODO: This method assumes the memory returned by realloc()
//       will have the same alignment as the previous call to malloc()
void MemoryPool::resize(size_t sz)
{
  auto ptr_offset   = m_ptr - m_pool;
  auto rover_offset = m_rover - m_pool;

  m_pool = (byte *)realloc(m_pool, sz);
  checkMalloc();

  m_ptr = m_pool + ptr_offset;
  m_rover = m_pool + rover_offset;
  m_end = m_pool + sz;
}

void *MemoryPool::ptr(Handle h)
{
  assertHandle(h);

  return m_ptr + h;
}

void MemoryPool::purge()
{
  m_rover = m_ptr;
}

void MemoryPool::purgeFrom(Handle where)
{
  assertHandle(where);

  m_rover = m_ptr + where;
}

size_t MemoryPool::size() const
{
  return m_end - m_ptr;
}

uintptr_t MemoryPool::align(uintptr_t ptr)
{
  // Align 'ptr' to AllocAlign-byte boundary
  return (ptr + (AllocAlign-1)) & ~AllocAlignMask;
}

void MemoryPool::assertHandle(Handle h)
{
  assert(h != Invalid && "Attempted to use an Invalid Handle!");
}

void MemoryPool::checkMalloc()
{
  if(!m_pool) throw MallocFailedEror();
}

}