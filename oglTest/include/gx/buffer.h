#pragma once

#include <gx/gx.h>

#include <cstdint>

#include <GL/glew.h>

namespace gx {

class Buffer {
public:
  enum Usage {
    Invalid,
    Static, Dynamic, Stream,
  };

  enum Access {
    Read      = GL_READ_ONLY,
    Write     = GL_WRITE_ONLY,
    ReadWrite = GL_READ_WRITE,
  };

  Buffer(const Buffer&) = delete;
  ~Buffer();

  virtual void use() const;

  template <typename T>
  void init(const T data[], size_t count) { init(data, sizeof(T), count); }
  template <typename T>
  void upload(const T data[], size_t offset, size_t count) { upload(data, offset, sizeof(T), count); }

  void init(size_t elem_sz, size_t elem_count);
  void init(const void *data, size_t elem_sz, size_t elem_count);
  void upload(const void *data, size_t offset, size_t elem_sz, size_t elem_count);

  void *map(Access access);
  void unmap();

  void label(const char *lbl);

protected:
  Buffer(Usage usage, GLenum target);

  GLenum usage() const;

  Usage m_usage;
  GLuint m;
  GLenum m_target;
};

template <typename T>
class BufferView {
public:
  BufferView(Buffer& buf, Buffer::Access access) :
    m(&buf), m_ptr(buf.map(access))
  { }
  BufferView(const BufferView& other) = delete;
  ~BufferView() { m->unmap(); }

  T* get(size_t offset) { return (T *)m_ptr + offset; }
  T& operator[](size_t offset) { return *get(offset); }

private:
  Buffer *m;
  void *m_ptr;
};

class VertexBuffer : public Buffer {
public:
  VertexBuffer(Usage usage);
};

class IndexBuffer : public Buffer {
public:
  IndexBuffer(Usage usage, Type type);

  virtual void use() const;

  Type elemType() const;
  unsigned elemSize() const;

private:
  Type m_type;
};

class UniformBuffer : public Buffer {
public:
  UniformBuffer(Usage usage);

  void bindToIndex(unsigned idx);
  void bindToIndex(unsigned idx, size_t offset, size_t size);
  void bindToIndex(unsigned idx, size_t size);
};

}
