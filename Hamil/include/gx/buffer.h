#pragma once

#include <gx/gx.h>

#include <cstdint>

namespace gx {

class Buffer {
public:
  enum Usage {
    Invalid,

    Static  = GL_STATIC_DRAW,
    Dynamic = GL_DYNAMIC_DRAW, 
    Stream  = GL_STREAM_DRAW,

    StaticRead  = GL_STATIC_READ,
    DynamicRead = GL_DYNAMIC_READ,
    StreamRead  = GL_STREAM_READ,

    StreamCopy = GL_STREAM_COPY,
  };

  enum Access : GLbitfield {
    Read      = GL_READ_ONLY,
    Write     = GL_WRITE_ONLY,
    ReadWrite = GL_READ_WRITE,
  };

  enum MapFlags : uint {
    Default = 0,

    Invalidate      = GL_MAP_INVALIDATE_BUFFER_BIT,
    InvalidateRange = GL_MAP_INVALIDATE_RANGE_BIT,
    Unsynchronized  = GL_MAP_UNSYNCHRONIZED_BIT,
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
  void *map(Access access, GLintptr off, GLint sz, uint flags = Default);
  void unmap();

  void flush(ssize_t off, ssize_t sz);

  void label(const char *lbl);

protected:
  Buffer(Usage usage, GLenum target);

  Usage m_usage;
  GLuint m;
  GLenum m_target;
};

template <typename T>
class BufferView {
public:
  BufferView(Buffer& buf, Buffer::Access access) :
    m(&buf), m_sz(-1),
    m_ptr(buf.map(access))
  { }
  BufferView(Buffer& buf,
    Buffer::Access access, ssize_t off, ssize_t sz, uint flags = Buffer::Default) :
    m(&buf), m_sz(sz),
    m_ptr(buf.map(access, off, sz, flags))
  { }
  BufferView(const BufferView& other) = delete;
  ~BufferView() { m->unmap(); }

  T *get(size_t offset) { return (T *)m_ptr + offset; }
  T& operator[](size_t offset) { return *get(offset); }

  void flush(ssize_t off = 0) { flush(off, m_sz); }
  void flush(ssize_t off, ssize_t sz = -1) { m->flush(off, sz); }

private:
  Buffer *m;

  ssize_t m_sz;
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
