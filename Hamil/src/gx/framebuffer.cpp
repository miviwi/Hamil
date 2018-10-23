#include <gx/framebuffer.h>

#include <cassert>
#include <algorithm>

namespace gx {

Framebuffer::Attachment Framebuffer::Color(int index)
{
  return (Attachment)(index < 0 ? ~0 : (Color0+index));
}

Framebuffer::Framebuffer() :
  m_samples(0),
  m_draw_buffers(DrawBuffersNeedSetup)
{
  glGenFramebuffers(1, &m);
}

Framebuffer::~Framebuffer()
{
  if(deref()) return;

  glDeleteFramebuffers(1, &m);
  glDeleteRenderbuffers((GLsizei)m_rb.size(), m_rb.data());
}

Framebuffer& Framebuffer::use()
{
  glBindFramebuffer(GL_FRAMEBUFFER, m);
  m_bound = GL_FRAMEBUFFER;

  setupDrawBuffers();

  return *this;
}

Framebuffer& Framebuffer::use(BindTarget target)
{
  glBindFramebuffer(m_bound, m);
  m_bound = (GLenum)target;

  setupDrawBuffers();

  return *this;
}

Framebuffer& Framebuffer::tex(const Texture2D& tex, unsigned level, Attachment att)
{
  m_samples = tex.m_samples;

  checkIfBound();
  glFramebufferTexture(m_bound, attachment(att), tex.m, level);
  drawBuffer(att);

  return *this;
}

Framebuffer& Framebuffer::tex(const Texture2DArray& tex, unsigned level, unsigned layer, Attachment att)
{
  checkIfBound();
  glFramebufferTextureLayer(m_bound, attachment(att), tex.m, level, layer);
  drawBuffer(att);

  return *this;
}

Framebuffer& Framebuffer::renderbuffer(Format fmt, Attachment att)
{
  auto dimensions = getColorAttachement0Dimensions();

  return renderbuffer(dimensions.x, dimensions.y, fmt, att);
}

Framebuffer& Framebuffer::renderbuffer(unsigned w, unsigned h, Format fmt, Attachment att)
{
  auto rb = create_rendebuffer();
  m_rb.append(rb);

  if(m_samples) {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, fmt, w, h);
  } else {
    glRenderbufferStorage(GL_RENDERBUFFER, fmt, w, h);
  }
  framebufferRenderbuffer(rb, att);
  drawBuffer(att);

  return *this;
}

Framebuffer& Framebuffer::renderbuffer(ivec2 sz, Format fmt, Attachment att)
{
  assert((sz.x >= 0 && sz.y >= 0) && "Attempted to create a renderbuffer with negative size!");

  return renderbuffer((unsigned)sz.x, (unsigned)sz.y, fmt, att);
}

Framebuffer& Framebuffer::renderbufferMultisample(unsigned samples, Format fmt, Attachment att)
{
  auto dimensions = getColorAttachement0Dimensions();

  return renderbufferMultisample(samples, dimensions.x, dimensions.y, fmt, att);
}

Framebuffer& Framebuffer::renderbufferMultisample(unsigned samples, unsigned w, unsigned h, Format fmt, Attachment att)
{
  auto rb = create_rendebuffer();
  m_rb.append(rb);

  m_samples = samples;

  glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, fmt, w, h);
  framebufferRenderbuffer(rb, att);
  drawBuffer(att);

  return *this;
}

Framebuffer& Framebuffer::renderbufferMultisample(unsigned samples, ivec2 sz, Format fmt, Attachment att)
{
  assert((sz.x >= 0 && sz.y >= 0) && "Attempted to create a renderbufferMultisample with negative size!");

  return renderbufferMultisample(samples, (unsigned)sz.x, (unsigned)sz.y, fmt, att);
}

void Framebuffer::blit(Framebuffer& fb, ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter)
{
  fb.use();
  use();

  doBlit(fb.m, src, dst, mask, filter);
}

void Framebuffer::blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter)
{
  use();

  doBlit(0, src, dst, mask, filter);
}

void Framebuffer::copy(Framebuffer& fb, ivec4 rect, unsigned mask, Sampler::Param filter)
{
  blit(fb, rect, rect, mask, filter);
}

void Framebuffer::clear(unsigned mask)
{
  use();

  glClear(mask);
}

Framebuffer::Status Framebuffer::status()
{
  use();
  return (Status)glCheckFramebufferStatus(m_bound);
}

bool Framebuffer::complete()
{
  return status() == Complete;
}

void Framebuffer::doBlit(GLuint fb, ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);

  GLenum f = -1;
  switch(filter) {
  case Sampler::Nearest: f = GL_NEAREST; break;
  case Sampler::Linear:  f = GL_LINEAR; break;

  default: f = GL_LINEAR; break;
  }

  glBlitFramebuffer(src.x, src.y, src.z, src.w, dst.x, dst.y, dst.z, dst.w, mask, f);

  glBindFramebuffer(m_bound, m);
}

void Framebuffer::bind_window(BindTarget target)
{
  glBindFramebuffer((GLenum)target, 0);
}

void Framebuffer::label(const char *label)
{
#if !defined(NDEBUG)
  glObjectLabel(GL_FRAMEBUFFER, m, -1, label);
#endif
}

GLenum Framebuffer::attachment(Attachment att)
{
  if(att >= Color0) return GL_COLOR_ATTACHMENT0 + (att-Color0);
  else if(att == Depth) return GL_DEPTH_ATTACHMENT;
  else if(att == DepthStencil) return GL_DEPTH_STENCIL_ATTACHMENT;

  return 0;
}

GLuint Framebuffer::create_rendebuffer()
{
  GLuint rb;
  glGenRenderbuffers(1, &rb);

  glBindRenderbuffer(GL_RENDERBUFFER, rb);
  return rb;
}

void Framebuffer::framebufferRenderbuffer(GLuint rb, Attachment att)
{
  checkIfBound();
  glFramebufferRenderbuffer(m_bound, attachment(att), GL_RENDERBUFFER, rb);
}

void Framebuffer::checkIfBound()
{
  GLint bound = -1;
  switch(m_bound) {
  case GL_READ_FRAMEBUFFER: glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &bound); break;
  default:                  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound); break;
  }

  if(bound != m) assert(0 && "Framebuffer needs to be bound before use");
}

ivec2 Framebuffer::getColorAttachement0Dimensions()
{
  GLint att_type = -1;
  glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &att_type);
  assert(att_type != GL_NONE && "INAVLID_OPERATION no Color0 attachment!");

  GLint name = -1;
  glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

  ivec2 dims{ -1, -1 };
  switch(att_type) {
  case GL_RENDERBUFFER:
    glBindRenderbuffer(GL_RENDERBUFFER, name);

    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &dims.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &dims.y);
    break;

  case GL_TEXTURE: {
    GLenum target = m_samples ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glBindTexture(target, name);

    // Check if we've chosen the target properly,
    //   if not retry with another
    if(glGetError() == GL_INVALID_OPERATION) {
      target = GL_TEXTURE_2D_ARRAY;
      glBindTexture(target, name);
    }

    int level = -1;
    glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);

    glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &dims.x);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &dims.y);
    break;
  }

  default: break;
  }
  return dims;
}

void Framebuffer::drawBuffer(Attachment att)
{
  if(att < Color0) return;

  unsigned idx = att - Color0;
  m_draw_buffers |= DrawBuffersNeedSetup | (1<<idx);
}

void Framebuffer::setupDrawBuffers()
{
  if(!(m_draw_buffers & DrawBuffersNeedSetup)) return;

  GLenum bufs[NumDrawBuffers];
  std::fill(bufs, bufs+NumDrawBuffers, GL_NONE);

  for(unsigned i = 0; i < NumDrawBuffers; i++) {
    bool enabled = (m_draw_buffers>>i) & 1;
    if(!enabled) continue;

    bufs[i] = GL_COLOR_ATTACHMENT0 + i;
  }

  glDrawBuffers(NumDrawBuffers, bufs);
  m_draw_buffers &= ~DrawBuffersNeedSetup;
}

}