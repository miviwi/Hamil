#pragma once

#include <cstdint>

#include <GL/glew.h>

namespace gx {

class Buffer {
public:
  enum Usage {
    Invalid,
    Static, Dynamic, Stream,
  };

  Buffer(const Buffer&) = delete;
  ~Buffer();

  template <typename T>
  void init(T data[], size_t count) { init(data, sizeof(T), count); }
  template <typename T>
  void upload(T data[], size_t offset, size_t count) { upload(data, offset, sizeof(T), count); }

  void init(void *data, size_t elem_sz, size_t elem_count);
  void upload(void *data, size_t offset, size_t elem_sz, size_t elem_count);

protected:
  Buffer(Usage usage, GLenum target);

  friend class VertexArray;
  friend class Program;

  GLenum usage() const;

  Usage m_usage;
  GLuint m;
  GLenum m_target;
};

class VertexBuffer : public Buffer {
public:
  VertexBuffer(Usage usage);
};

class IndexBuffer : public Buffer {
public:
  enum Type {
    u8 = GL_UNSIGNED_BYTE,
    u16 = GL_UNSIGNED_SHORT,
    u32 = GL_UNSIGNED_INT,
  };

  IndexBuffer(Usage usage, Type type);

private:
  friend class Program;

  Type m_type;
};


}
