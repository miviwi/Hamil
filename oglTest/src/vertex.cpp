#include "vertex.h"
#include "buffer.h"

namespace ogl {

VertexFormat& VertexFormat::attr(AttributeType type, unsigned size, bool normalized)
{
  m_descs.push_back({ type, size, false, normalized });

  return *this;
}

VertexFormat& VertexFormat::iattr(AttributeType type, unsigned size)
{
  m_descs.push_back({ type, size, true, false });

  return *this;
}

size_t VertexFormat::vertextByteSize() const
{
  size_t sz = 0;
  for(auto& desc : m_descs) sz += byteSize(desc);

  return sz;
}

size_t VertexFormat::numAttrs() const
{
  return m_descs.size();
}

void *VertexFormat::attrOffset(unsigned idx) const
{
  size_t off = 0;
  auto it = m_descs.cbegin();
  
  while(idx--) {
    off += byteSize(*it);

    it++;
  }

  return (void *)off;
}

GLint VertexFormat::attrSize(unsigned idx) const
{
  return (GLint)m_descs[idx].size;
}

GLenum VertexFormat::attrType(unsigned idx) const
{
  static const GLenum table[] = {
    GL_BYTE, GL_UNSIGNED_BYTE,
    GL_SHORT, GL_UNSIGNED_SHORT,
    GL_INT, GL_UNSIGNED_INT,
    GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE,
    GL_FIXED,
  };
  return table[m_descs[idx].type];
}

GLboolean VertexFormat::attrNormalized(unsigned idx) const
{
  return m_descs[idx].normalize ? GL_TRUE : GL_FALSE;
}

bool VertexFormat::attrInteger(unsigned idx) const
{
  return m_descs[idx].integer;
}

size_t VertexFormat::byteSize(Desc desc)
{
  static const size_t table[] = {
    1, 1, 2, 2, 4, 4,
    2, 4, 8,
    4,
  };
  return table[desc.type]*desc.size;
}

VertexArray::VertexArray(const VertexFormat& fmt, const VertexBuffer& buf)
{
  glGenVertexArrays(1, &m);

  glBindBuffer(GL_ARRAY_BUFFER, buf.m);
  glBindVertexArray(m);

  for(unsigned i = 0; i < fmt.numAttrs(); i++) {
    glEnableVertexAttribArray(i);

    if(fmt.attrInteger(i)) {
      glVertexAttribIPointer(i, fmt.attrSize(i), fmt.attrType(i), fmt.vertextByteSize(), fmt.attrOffset(i));
    } else {
      glVertexAttribPointer(i, fmt.attrSize(i), fmt.attrType(i), fmt.attrNormalized(i),
                            fmt.vertextByteSize(), fmt.attrOffset(i));
    }
  }
}

VertexArray::~VertexArray()
{
  glDeleteVertexArrays(1, &m);
}

void VertexArray::use()
{
  glBindVertexArray(m);
}

}
