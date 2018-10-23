#include <gx/buffer.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>

#include <cassert>
#include <cstring>

namespace gx {

Buffer::Buffer(Usage usage, GLenum target) :
  m_usage(usage), m_target(target)
{
  glGenBuffers(1, &m);
}

Buffer::~Buffer()
{
  if(deref()) return;

  glDeleteBuffers(1, &m);
}

BufferView Buffer::map(Access access)
{
  use();
  auto ptr = glMapBuffer(m_target, access);

  return BufferView(*this, m_target, ptr);
}

BufferView Buffer::map(Access access, GLintptr off, GLint sz, uint flags)
{
  use();

  GLbitfield gl_access = 0;
  switch(access) {
  case Read:  gl_access = GL_MAP_READ_BIT; break;
  case Write: gl_access = GL_MAP_WRITE_BIT; break;

  case ReadWrite: gl_access = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT; break;
  }

  auto ptr = glMapBufferRange(m_target, off, sz, gl_access | (GLbitfield)flags);
  if(!ptr) {
    // It's illegal to { MapInvalidate, MapInvalidateRange, MapUnsynchronized }
    //   a buffer intended for reading
    if(gl_access & GL_MAP_READ_BIT && flags != MapDefault) throw InvalidMapFlagsError();

    // Unknown error
    throw MapError();
  }

  return BufferView(*this, m_target, ptr, sz);
}

void Buffer::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_BUFFER, m, -1, lbl);
#endif
}

void Buffer::use() const
{
  glBindBuffer(m_target, m);
}

void Buffer::init(size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, nullptr, m_usage);
}

void Buffer::init(const void *data, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, data, m_usage);
}

void Buffer::upload(const void *data, size_t offset, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferSubData(m_target, offset*elem_sz, sz, data);
}

BufferView::BufferView(const Buffer& buf, GLenum target, void *ptr, ssize_t sz) :
  m(buf), m_target(target), m_sz(sz), m_ptr(ptr)
{
}

BufferView::~BufferView()
{
  if(deref()) return;

  unmap();
}

void *BufferView::get() const
{
  return m_ptr;
}

uint8_t& BufferView::operator[](size_t offset)
{
  return *(get<uint8_t>() + offset);
}

void BufferView::flush(ssize_t off)
{
  flush(off, m_sz);
}

void BufferView::flush(ssize_t off, ssize_t sz)
{
  m.use();

  assert(sz > 0 && "Attempting to flush a mapping with unknown size without specifying one!");
  glFlushMappedBufferRange(m_target, off, sz);
}

void BufferView::unmap()
{
  m.use();

  auto result = glUnmapBuffer(m_target);
  assert(result && "failed to unmap buffer!");

  m_target = GL_INVALID_ENUM;

  m_ptr = nullptr;
  m_sz = -1;
}

VertexBuffer::VertexBuffer(Usage usage) :
  Buffer(usage, GL_ARRAY_BUFFER)
{
}

IndexBuffer::IndexBuffer(Usage usage, Type type) :
  Buffer(usage, GL_ELEMENT_ARRAY_BUFFER), m_type(type)
{
}

void IndexBuffer::use() const
{
  Buffer::use();
}

Type IndexBuffer::elemType() const
{
  return m_type;
}

unsigned IndexBuffer::elemSize() const
{
  switch(m_type) {
  case u8:  return 1;
  case u16: return 2;
  case u32: return 4;
  }

  return 0;
}

static GLuint p_last_uniform[NumUniformBindings] = {
  ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
  ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
  ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
};

UniformBuffer::UniformBuffer(Usage usage) :
  Buffer(usage, GL_UNIFORM_BUFFER)
{
}

void UniformBuffer::bindToIndex(unsigned idx)
{
  if(p_last_uniform[idx] == m) return;

  p_last_uniform[idx] = m;
  glBindBufferBase(m_target, idx, m);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t offset, size_t size)
{
  if(p_last_uniform[idx] == m) return;

  p_last_uniform[idx] = m;
  glBindBufferRange(m_target, idx, m, offset, size);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t size)
{
  bindToIndex(idx, 0, size);
}

TexelBuffer::TexelBuffer(Usage usage) :
  Buffer(usage, GL_TEXTURE_BUFFER)
{
}

PixelBuffer::PixelBuffer(Usage usage, TransferDirection xfer_dir) :
  Buffer(usage, target_for_xfer_direction(xfer_dir))
{
}

void PixelBuffer::uploadTexture(Texture& tex,
  unsigned mip, unsigned w, unsigned h, Format fmt, Type type, size_t off)
{
  assertUpload();

  use();
  tex.use();

  glTexImage2D(tex.m_target, mip, tex.m_format, w, h, 0, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

void PixelBuffer::uploadTexture(Texture& tex,
  unsigned mip, unsigned w, unsigned h, unsigned d, Format fmt, Type type, size_t off)
{
  assertUpload();

  use();
  tex.use();

  glTexImage3D(tex.m_target, mip, tex.m_format, w, h, d, 0, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

void PixelBuffer::uploadTexture(Texture& tex,
  unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h, Format fmt, Type type, size_t off)
{
  assertUpload();

  use();
  tex.use();

  glTexSubImage2D(tex.m_target, mip, x, y, w, h, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

void PixelBuffer::uploadTexture(Texture& tex,
  unsigned mip, unsigned x, unsigned y, unsigned z, unsigned w, unsigned h, unsigned d,
  Format fmt, Type type, size_t off)
{
  assertUpload();

  use();
  tex.use();

  glTexSubImage3D(tex.m_target, mip, x, y, z, w, h, d, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

void PixelBuffer::downloadTexture(Texture& tex, unsigned mip, Format fmt, Type type, size_t off)
{
  assertDownload();

  use();
  tex.use();

  glGetTexImage(tex.m_target, mip, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

void PixelBuffer::downloadFramebuffer(Framebuffer& fb,
  int w, int h, Format fmt, Type type, unsigned attachment, size_t off)
{
  downloadFramebuffer(fb, 0, 0, w, h, fmt, type, attachment, off);
}

void PixelBuffer::downloadFramebuffer(Framebuffer& fb,
  int x, int y, int w, int h, Format fmt, Type type, unsigned attachment, size_t off)
{
  assertDownload();

  use();
  fb.use(gx::Framebuffer::Read);

  glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment);
  glReadPixels(x, y, w, h, fmt, type, (void *)(uintptr_t)off);

  unbind();
}

PixelBuffer& PixelBuffer::transferDirection(TransferDirection xfer_dir)
{
  m_target = target_for_xfer_direction(xfer_dir);
  return *this;
}

GLenum PixelBuffer::target_for_xfer_direction(TransferDirection xfer_dir)
{
  switch(xfer_dir) {
  case Upload:   return GL_PIXEL_UNPACK_BUFFER;
  case Download: return GL_PIXEL_PACK_BUFFER;
  }

  return GL_INVALID_ENUM;
}

void PixelBuffer::assertUpload()
{
  assert(m_target == GL_PIXEL_UNPACK_BUFFER && "Attempted an Upload operation with a Download PixelBufer!");
}

void PixelBuffer::assertDownload()
{
  assert(m_target == GL_PIXEL_PACK_BUFFER && "Attempted a Download operation with an Upload PixelBufer!");
}

void PixelBuffer::unbind()
{
  // Unbind the PBO so further texture operations proceed normally
  glBindBuffer(m_target, 0);
}

BufferHandle::BufferHandle(Buffer *buf) :
  m(buf)
{
}

BufferHandle::~BufferHandle()
{
  if(deref()) return;

  delete m;
}

Buffer& BufferHandle::get()
{
  return *m;
}

Buffer& BufferHandle::operator()()
{
  return get();
}

void BufferHandle::label(const char *lbl)
{
  get().label(lbl);
}

void BufferHandle::use()
{
  get().use();
}

}
