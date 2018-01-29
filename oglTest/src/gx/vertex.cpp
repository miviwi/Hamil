#include <gx/vertex.h>
#include <gx/buffer.h>

#include <cstring>

namespace gx {

GLuint p_last_array = ~0u;

VertexFormat& VertexFormat::attr(Type type, unsigned size, bool normalized)
{
  m_descs.push_back({ NextIndex, type, size, normalized ? Normalize : 0 });

  return *this;
}

VertexFormat& VertexFormat::iattr(Type type, unsigned size)
{
  m_descs.push_back({ NextIndex, type, size, Integer });

  return *this;
}

VertexFormat& VertexFormat::attrAlias(unsigned index, Type type, unsigned size, bool normalized)
{
  unsigned flags = normalized ? Normalize : 0;
  m_descs.push_back({ index, type, size,  flags | Alias });

  return *this;
}

size_t VertexFormat::vertexByteSize() const
{
  size_t sz = 0;
  for(auto& desc : m_descs) {
    if(desc.index == NextIndex) sz += byteSize(desc);
  }

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

  if(m_descs[idx].flags & Alias) idx = m_descs[idx].index;
  
  while(idx--) {
    if(it->index == NextIndex) off += byteSize(*it);

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
  return m_descs[idx].type;
}

GLboolean VertexFormat::attrNormalized(unsigned idx) const
{
  return m_descs[idx].flags & Normalize ? GL_TRUE : GL_FALSE;
}

bool VertexFormat::attrInteger(unsigned idx) const
{
  return m_descs[idx].flags & Integer;
}

size_t VertexFormat::byteSize(Desc desc)
{
  size_t size = desc.size;
  switch(desc.type) {
  case i8:
  case u8:  size *= 1; break;
  case f16:
  case i16:
  case u16: size *= 2; break;
  case f32:
  case i32:
  case u32: size *= 4; break;
  case f64: size *= 8; break;

  case fixed: size *= 4; break;
  }
  return size;
}

VertexArray::VertexArray(const VertexFormat& fmt, const VertexBuffer& buf) :
  m_elem_size(fmt.vertexByteSize())
{
  glGenVertexArrays(1, &m);

  glBindBuffer(GL_ARRAY_BUFFER, buf.m);
  glBindVertexArray(m);

  init(fmt);
}

VertexArray::~VertexArray()
{
  glDeleteVertexArrays(1, &m);
}

unsigned VertexArray::elemSize() const
{
  return m_elem_size;
}

void VertexArray::use() const
{
  if(p_last_array != m) glBindVertexArray(m);

  p_last_array = m;
}

void VertexArray::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_VERTEX_ARRAY, m, strlen(lbl), lbl);
#endif
}

void VertexArray::init(const VertexFormat& fmt)
{
  for(unsigned i = 0; i < fmt.numAttrs(); i++) {
    GLint size = fmt.attrSize(i);
    GLenum type = fmt.attrType(i);
    GLboolean normalized = fmt.attrNormalized(i);
    void *offset = fmt.attrOffset(i);

    glEnableVertexAttribArray(i);

    if(fmt.attrInteger(i)) {
      glVertexAttribIPointer(i, size, type, m_elem_size, offset);
    } else {
      glVertexAttribPointer(i, size, type, normalized, m_elem_size, offset);
    }
  }
}

}
