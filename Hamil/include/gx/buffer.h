#pragma once

#include <gx/gx.h>

#include <util/ref.h>

#include <cstdint>

namespace gx {

class Texture;
class Framebuffer;

class BufferView;

class Buffer : public Ref {
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
    MapDefault = 0,

    MapInvalidate      = GL_MAP_INVALIDATE_BUFFER_BIT,
    MapInvalidateRange = GL_MAP_INVALIDATE_RANGE_BIT,
    MapUnsynchronized  = GL_MAP_UNSYNCHRONIZED_BIT,
  };

  ~Buffer();

  virtual void use() const;

  template <typename T>
  void init(const T data[], size_t count) { init(data, sizeof(T), count); }
  template <typename T>
  void upload(const T data[], size_t offset, size_t count) { upload(data, offset, sizeof(T), count); }

  void init(size_t elem_sz, size_t elem_count);
  void init(const void *data, size_t elem_sz, size_t elem_count);
  void upload(const void *data, size_t offset, size_t elem_sz, size_t elem_count);

  BufferView map(Access access);
  BufferView map(Access access, GLintptr off, GLint sz, uint flags = MapDefault);

  void label(const char *lbl);

protected:
  Buffer(Usage usage, GLenum target);

  Usage m_usage;
  GLuint m;
  GLenum m_target;
};

class BufferView : public Ref {
public:
  ~BufferView();

  void *get() const;
  template <typename T> T *get() const { return (T *)get(); }
  uint8_t& operator[](size_t offset);

  void flush(ssize_t off = 0);
  void flush(ssize_t off, ssize_t sz = -1);

  void unmap();

protected:
  BufferView(const Buffer& buf, GLenum target, void *ptr, ssize_t sz = -1);

private:
  friend Buffer;

  Buffer m;
  GLenum m_target;

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

// PixelBuffer(..., Upload)   -> The buffer will be a source of Texture data
// PixelBuffer(..., Download) -> The buffer will contain read Texture/Framebuffer data
class PixelBuffer : public Buffer {
public:
  enum TransferDirection {
    Upload, Download,
  };

  PixelBuffer(Usage usage, TransferDirection xfer_dir);

  // Invalidates current Texture binding!
  void uploadTexture(Texture& tex, unsigned mip, unsigned w, unsigned h,
    Format fmt, Type type, size_t off = 0);
  // Invalidates current Texture binding!
  void uploadTexture(Texture& tex, unsigned mip, unsigned w, unsigned h, unsigned d,
    Format fmt, Type type, size_t off = 0);

  // Invalidates current Texture binding!
  //   - Must call init() to allocate storage before using this method!
  void uploadTexture(Texture& tex, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
    Format fmt, Type type, size_t off = 0);
  // Invalidates current Texture binding!
  //   - Must call init() to allocate storage before using this method!
  void uploadTexture(Texture& tex, unsigned mip,
    unsigned x, unsigned y, unsigned z, unsigned w, unsigned h, unsigned d,
    Format fmt, Type type, size_t off = 0);

  // Invalidates current Texture binding!
  //   - Downloads the WHOLE texture to the buffer
  void downloadTexture(Texture& tex, unsigned mip, Format fmt, Type type, size_t off = 0);

  // Invalidates current Framebuffer::Read binding!
  void downloadFramebuffer(Framebuffer& fb, int w, int h,
    Format fmt, Type type, unsigned attachment = 0, size_t off = 0);

  // Invalidates current Framebuffer::Read binding!
  void downloadFramebuffer(Framebuffer& fb, int x, int y, int w, int h,
    Format fmt, Type type, unsigned attachment = 0, size_t off = 0);

  PixelBuffer& transferDirection(TransferDirection xfer_dir);

private:
  static GLenum target_for_xfer_direction(TransferDirection xfer_dir);

  void assertUpload();
  void assertDownload();

  void unbind();
};

}
