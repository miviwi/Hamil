#include <gx/buffer.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>
#include <gx/info.h>

#include <cassert>
#include <cstring>
#include <algorithm>

namespace gx {

Buffer::Buffer(Usage usage, GLenum target) :
  m_usage(usage), m_target(target),
  m_sz(-1)
{
  glGenBuffers(1, &m);
}

void Buffer::assertValid() const
{
  assert(m_usage != Invalid && "Attempted to use invalid Buffer!");
}

Buffer::~Buffer()
{
  if(deref() || /* destroy() was called */ !m) return;

  glDeleteBuffers(1, &m);
}

BufferView Buffer::map(Access access, uint flags)
{
  use();
  void *ptr = nullptr;

  if(flags == MapDefault && m_sz < 0) {
    // 'flags' are ignored if the Buffer's size is unknown
    ptr = glMapBuffer(m_target, access);
  } else {
    // Delegate to glMapBufferRange() to honor 'flags'
    return map(access, 0, (GLint)m_sz, flags);
  }

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

void Buffer::destroy()
{
  glDeleteBuffers(1, &m);

  m_usage = Invalid;
  m = 0;
  m_sz = -1;
}

void Buffer::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_BUFFER, m, -1, lbl);
  unbind();
#endif
}

void Buffer::use() const
{
  assertValid();

  glBindBuffer(m_target, m);
}

void Buffer::unbind() const
{
  // No-op for everything except PixelBuffer
}

void Buffer::init(size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, nullptr, m_usage);

  m_sz = (ssize_t)sz;

  unbind();
}

void Buffer::init(const void *data, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferData(m_target, sz, data, m_usage);

  m_sz = (ssize_t)sz;

  unbind();
}

void Buffer::upload(const void *data, size_t offset, size_t elem_sz, size_t elem_count)
{
  size_t sz = elem_sz*elem_count;

  use();
  glBufferSubData(m_target, offset*elem_sz, sz, data);

  unbind();
}

void Buffer::copy(Buffer& dst, size_t src_offset, size_t dst_offset, size_t sz)
{
  glBindBuffer(GL_COPY_READ_BUFFER, m);
  glBindBuffer(GL_COPY_WRITE_BUFFER, dst.m);

  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    (GLintptr)src_offset, (GLintptr)dst_offset, (GLsizeiptr)sz);
}

void Buffer::copy(Buffer& dst, size_t sz)
{
  copy(dst, 0, 0, sz);
}

BufferView::BufferView(const Buffer& buf, GLenum target, void *ptr, ssize_t sz) :
  m(buf), m_target(target), m_sz(sz), m_ptr(ptr)
{
  buf.unbind();
}

BufferView::~BufferView()
{
  if(deref()) return;

  // Check if unmap() hasn't been called explicilty
  if(m_ptr) unmap();
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

  m.unbind();
}

void BufferView::unmap()
{
  m.use();

  auto result = glUnmapBuffer(m_target);
  assert(result && "failed to unmap buffer!");

  m_target = GL_INVALID_ENUM;

  m_ptr = nullptr;
  m_sz = -1;

  m.unbind();
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

struct pBufferBinding {
  GLuint buf = ~0u;
  size_t offset = 0, size = ~0ull;
};

thread_local static pBufferBinding p_last_uniform[NumUniformBindings];

UniformBuffer::UniformBuffer(Usage usage) :
  Buffer(usage, GL_UNIFORM_BUFFER)
{
}

UniformBuffer::~UniformBuffer()
{
  // ~Buffer() will call deref()
  if(refs() > 1) return;

  // When a Buffer gets destroyed the indexed bindings
  //   get invalidated, so clear the shadowed state
  clearBindings();
}

void UniformBuffer::bindToIndex(unsigned idx)
{
  if(checkBinding(idx)) return;

  updateBinding(idx);
  glBindBufferBase(m_target, idx, m);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t offset, size_t size)
{
  assert((offset % info().minUniformBindAlignment() == 0 &&
    size % info().minUniformBindAlignment() == 0) &&
    "The offset or size passed to bindToIndex() do not meet the alignment requirement!");

  if(checkBinding(idx, offset, size)) return;

  updateBinding(idx, offset, size);
  glBindBufferRange(m_target, idx, m, offset, size);
}

void UniformBuffer::bindToIndex(unsigned idx, size_t size)
{
  bindToIndex(idx, 0, size);
}

bool UniformBuffer::checkBinding(unsigned idx, size_t offset, size_t size)
{
  const auto& binding = p_last_uniform[idx];
  if(binding.buf != m || binding.offset != offset || binding.size < size) return false;

  return true;
}

void UniformBuffer::updateBinding(unsigned idx, size_t offset, size_t size)
{
  auto& binding = p_last_uniform[idx];

  binding.buf = m;
  binding.offset = offset;
  binding.size   = size;
}

void UniformBuffer::clearBindings()
{
  auto it = p_last_uniform;
  auto end = p_last_uniform + NumUniformBindings;
  while(it != end) {
    it = std::find_if(it, end, [this](const pBufferBinding& binding) {
      return binding.buf == m;
    });

    // The Buffer was never bound to an index
    if(it == end) break;

    // Clear the binding
    it->buf = ~0u;
  }
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
  assert(m_target == GL_PIXEL_UNPACK_BUFFER && "Attempted an Upload operation on a Download PixelBufer!");
}

void PixelBuffer::assertDownload()
{
  assert(m_target == GL_PIXEL_PACK_BUFFER && "Attempted a Download operation on an Upload PixelBufer!");
}

void PixelBuffer::unbind() const
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

}
