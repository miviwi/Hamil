#include "buffer.h"

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

void Buffer::init(void *data, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  glBindBuffer(m_target, m);
  glBufferData(m_target, sz, data, usage());
}

void Buffer::upload(void *data, size_t offset, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  glBindBuffer(m_target, m);
  glBufferSubData(m_target, offset, sz, data);
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

}
