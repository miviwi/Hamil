#include <ek/constbuffer.h>

#include <gx/resourcepool.h>
#include <gx/buffer.h>

#include <utility>

namespace ek {

ConstantBuffer::ConstantBuffer(u32 id, size_t sz) :
  m_id(id), m_sz(sz)
{
}

ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) :
  SharedObject(std::move(other)),
  m_id(other.m_id), m_sz(other.m_sz)
{
  other.m_id = gx::ResourcePool::Invalid;
  other.m_sz = 0;
}

u32 ConstantBuffer::id() const
{
  return m_id;
}

gx::UniformBuffer& ConstantBuffer::get(gx::ResourcePool& pool) const
{
  return pool.getBuffer<gx::UniformBuffer>(m_id);
}

size_t ConstantBuffer::size() const
{
  return m_sz;
}

}