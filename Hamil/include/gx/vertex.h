#pragma once

#include <gx/gx.h>

#include <util/ref.h>

#include <cstdint>
#include <vector>

namespace gx {

class VertexBuffer;
class IndexBuffer;

class VertexFormat {
public:
  enum IsNormalized {
    UnNormalized = false, Normalized = true,
  };

  VertexFormat& attr(Type type, unsigned size, IsNormalized normalized = Normalized);
  VertexFormat& iattr(Type type, unsigned size);

  VertexFormat& attrAlias(unsigned index, Type type, unsigned size, IsNormalized normalized = Normalized);

  size_t vertexByteSize() const;

  size_t numAttrs() const;

  void *attrOffset(unsigned idx) const;
  GLint attrSize(unsigned idx) const;
  GLenum attrType(unsigned idx) const;
  GLboolean attrNormalized(unsigned idx) const;
  bool attrInteger(unsigned idx) const;

private:
  friend class VertexArray;

  enum : unsigned {
     NextIndex = ~0u,

     Normalize = 1<<0,
     Integer   = 1<<1,
     Alias     = 1<<2,
  };
  struct Desc {
    unsigned index;

    Type type;
    unsigned size;
    unsigned flags;
  };

  static size_t byteSize(Desc desc);

  std::vector<Desc> m_descs;
};

class VertexArray : public Ref {
public:
  VertexArray(const VertexFormat& fmt, const VertexBuffer& buf);
  ~VertexArray();

  size_t elemSize() const;

  void use() const;
  // Needs to be called after indexed drawing!
  // (Could be done implicitly but is not for efficiency reasons)
  void end() const;

  void label(const char *lbl);

protected:
  static void unbind();

  VertexArray();

  void create();
  void init(const VertexFormat& fmt);

  GLuint m;
  size_t m_elem_size;
};

class IndexedVertexArray : public VertexArray {
public:
  IndexedVertexArray(const VertexFormat& fmt, const VertexBuffer& buf, const IndexBuffer& ind);

  Type indexType() const;
  unsigned indexSize() const;

private:
  Type m_index_type;
  unsigned m_index_size;
};

}
