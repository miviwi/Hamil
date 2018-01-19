#pragma once

#include <cstdint>

#include <GL/glew.h>

#include <vector>

namespace gx {

class Buffer;

class VertexFormat {
public:
  enum AttributeType {
    i8, u8, i16, u16, i32, u32,
    f16, f32, f64,
    fixed,
  };

  VertexFormat& attr(AttributeType type, unsigned size, bool normalized = true);
  VertexFormat& iattr(AttributeType type, unsigned size);

  VertexFormat& attrAlias(unsigned index, AttributeType type, unsigned size, bool normalized = true);

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

    AttributeType type;
    unsigned size;
    unsigned flags;
  };

  static size_t byteSize(Desc desc);

  std::vector<Desc> m_descs;
};

class VertexArray {
public:
  VertexArray(const VertexFormat& fmt, const Buffer& buf);
  VertexArray(const VertexArray&) = delete;
  ~VertexArray();

  unsigned elemSize() const;

  void use() const;

  void label(const char *lbl);

private:
  friend class Program;

  GLuint m;
  unsigned m_elem_size;
};

}
