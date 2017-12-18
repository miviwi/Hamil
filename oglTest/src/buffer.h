#pragma once

#include <cstdint>

#include <GL/glew.h>

namespace gx {

class VertexBuffer {
public:
  enum Usage {
    Invalid,
    Static, Dynamic, Stream,
  };

  VertexBuffer(Usage usage);
  VertexBuffer(const VertexBuffer&) = delete;
  ~VertexBuffer();

  template <typename T>
  void init(T data[], size_t count) { init(data, sizeof(T), count); }
  template <typename T>
  void upload(T data[], size_t offset, size_t count) { upload(data, offset, sizeof(T), count); }

  void init(void *data, size_t elem_sz, size_t elem_count);
  void upload(void *data, size_t offset, size_t elem_sz, size_t elem_count);

private:
  friend class VertexArray;

  GLenum usage() const;

  Usage m_usage;
  GLuint m;
};

}
