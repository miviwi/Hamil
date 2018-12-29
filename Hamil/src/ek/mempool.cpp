#include <ek/mempool.h>

#include <gx/memorypool.h>

namespace ek {

MemoryPool::MemoryPool(size_t size) :
  m_pool(new gx::MemoryPool(size))
{
}

gx::MemoryPool& MemoryPool::get()
{
  return *m_pool;
}

const gx::MemoryPool& MemoryPool::get() const
{
  return *m_pool;
}

gx::MemoryPool& MemoryPool::operator()()
{
  return get();
}

gx::MemoryPool *MemoryPool::ptr()
{
  return m_pool;
}

}