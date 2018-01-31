#include <gx/buffer.h>

#include <cassert>
#include <cstring>

namespace gx {

Buffer::Buffer(Usage usage,GLenum target) :
  m_usage(usage), m_target(target)
{
  glGenBuffers(1, &m);
}

Buffer::~Buffer()
{
  glDeleteBuffers(1, &m);
}

void *Buffer::map(Access access)
{
  use();

  return glMapBuffer(m_target, access);
}

void Buffer::unmap()
{
  use();

  auto result = glUnmapBuffer(m_target);
  assert(result && "failed to unmap buffer!");
}

void Buffer::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_BUFFER, m, strlen(lbl), lbl);
#endif
}

void Buffer::use() const
{
  glBindBuffer(m_target, m);
}

void Buffer::init(size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, nullptr, usage());
}

void Buffer::init(void *data, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, data, usage());
}

void Buffer::upload(void *data, size_t offset, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferSubData(m_target, offset*elem_sz, sz, data);
}

GLenum Buffer::usage() const
{
  static const GLenum table[] = {
    ~0u,
    GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,
  };
  return table[m_usage];
}

VertexBuffer::VertexBuffer(Usage usage) :
  Buffer(usage, GL_ARRAY_BUFFER)
{
}

IndexBuffer::IndexBuffer(Usage usage, Type type) :
  Buffer(usage, GL_ELEMENT_ARRAY_BUFFER), m_type(type)
{
}

void IndexBuffer::use() const
{
  Buffer::use();
}

Type IndexBuffer::elemType() const
{
  return m_type;
}

unsigned IndexBuffer::elemSize() const
{
  switch(m_type) {
  case u8:  return 1;
  case u16: return 2;
  case u32: return 4;
  }

  return 0;
}

UniformBuffer::UniformBuffer(Usage usage) :
  Buffer(usage, GL_UNIFORM_BUFFER)
{
}

void UniformBuffer::bindToIndex(unsigned idx)
{
  glBindBufferBase(m_target, idx, m);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t offset, size_t size)
{
  glBindBufferRange(m_target, idx, m, offset, size);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t size)
{
  bindToIndex(idx, 0, size);
}

}
