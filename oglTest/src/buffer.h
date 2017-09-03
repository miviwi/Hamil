#pragma once

#include <cstdint>

#include <GL/glew.h>

namespace ogl {

class VertexBuffer {
public:
  enum Usage {
    Invalid,
    Static, Dynamic, Stream,
  };

  VertexBuffer(Usage usage);
  VertexBuffer(const VertexBuffer&) = delete;
  ~VertexBuffer();

  void init(void *data, size_t elem_sz, size_t elem_count);
  void upload(void *data, size_t offset, size_t elem_sz, size_t elem_count);

private:
  friend class VertexArray;

  GLenum glUsage() const;

  Usage m_usage;
  GLuint m;
};

}
