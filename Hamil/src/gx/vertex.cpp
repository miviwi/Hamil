#include <gx/vertex.h>
#include <gx/buffer.h>

#include <cstring>

namespace gx {

VertexFormat& VertexFormat::attr(Type type, unsigned size, IsNormalized normalized)
{
  m_descs.push_back({ NextIndex, type, size, normalized ? Normalize : 0 });

  return *this;
}

VertexFormat& VertexFormat::iattr(Type type, unsigned size)
{
  m_descs.push_back({ NextIndex, type, size, Integer });

  return *this;
}

VertexFormat& VertexFormat::attrAlias(unsigned index, Type type, unsigned size, IsNormalized normalized)
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
  create();

  use();
  buf.use();
  init(fmt);

  unbind();
}

VertexArray::~VertexArray()
{
  if(deref()) return;

  glDeleteVertexArrays(1, &m);
}

size_t VertexArray::elemSize() const
{
  return m_elem_size;
}

void VertexArray::use() const
{
  p_bind_VertexArray(m);
}

void VertexArray::end() const
{
  unbind();
}

void VertexArray::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_VERTEX_ARRAY, m, -1, lbl);
  unbind();
#endif
}

void VertexArray::unbind()
{
  p_unbind_VertexArray();
}

VertexArray::VertexArray()
{
}

void VertexArray::create()
{
  unbind();
  glGenVertexArrays(1, &m);
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
      glVertexAttribIPointer(i, size, type, (GLsizei)m_elem_size, offset);
    } else {
      glVertexAttribPointer(i, size, type, normalized, (GLsizei)m_elem_size, offset);
    }
  }
}

IndexedVertexArray::IndexedVertexArray(const VertexFormat& fmt,
                                       const VertexBuffer& buf, const IndexBuffer& ind) :
  m_index_type(ind.elemType()), m_index_size(ind.elemSize())
{
  m_elem_size = fmt.vertexByteSize();

  create();

  use();
  buf.use();
  ind.use();
  init(fmt);

  unbind();
}

Type IndexedVertexArray::indexType() const
{
  return m_index_type;
}

unsigned IndexedVertexArray::indexSize() const
{
  return m_index_size;
}

}
