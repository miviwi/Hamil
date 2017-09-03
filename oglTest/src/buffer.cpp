#include "buffer.h"

namespace ogl {

VertexBuffer::VertexBuffer(Usage usage) :
  m_usage(usage)
{
  glGenBuffers(1, &m);
}

VertexBuffer::~VertexBuffer()
{
  glDeleteBuffers(1, &m);
}

void VertexBuffer::init(void *data, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  glBindBuffer(GL_ARRAY_BUFFER, m);
  glBufferData(GL_ARRAY_BUFFER, sz, data, glUsage());
}

void VertexBuffer::upload(void *data, size_t offset, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  glBindBuffer(GL_ARRAY_BUFFER, m);
  glBufferSubData(GL_ARRAY_BUFFER, offset, sz, data);
}

GLenum VertexBuffer::glUsage() const
{
  static const GLenum table[] = {
    ~0u,
    GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,
  };
  return table[m_usage];
}

}
