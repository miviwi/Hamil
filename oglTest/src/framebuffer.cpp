#include "framebuffer.h"

namespace gx {

Framebuffer::Framebuffer()
{
  glGenFramebuffers(1, &m);
}

Framebuffer::~Framebuffer()
{
  glDeleteFramebuffers(1, &m);
  glDeleteRenderbuffers(m_rb.size(), m_rb.data());
}

void Framebuffer::use()
{
  glBindFramebuffer(GL_FRAMEBUFFER, m);

  m_bound = GL_FRAMEBUFFER;
}

void Framebuffer::use(BindTarget target)
{
  m_bound = binding(target);

  glBindFramebuffer(m_bound, m);
}

void Framebuffer::tex(const Texture2D& tex, unsigned level, Attachment att)
{
  checkIfBound();

  glFramebufferTexture(m_bound, attachement(att), tex.m, level);
}

void Framebuffer::renderbuffer(Format fmt, Attachment att)
{
  auto dimensions = getColorAttachementDimensions();

  renderbuffer(dimensions.x, dimensions.y, fmt, att);
}

void Framebuffer::renderbuffer(unsigned w, unsigned h, Format fmt, Attachment att)
{
  GLuint rb;
  glGenRenderbuffers(1, &rb);

  glBindRenderbuffer(GL_RENDERBUFFER, rb);
  glRenderbufferStorage(GL_RENDERBUFFER, internalformat(fmt), w, h);

  checkIfBound();
  glFramebufferRenderbuffer(m_bound, attachement(att), GL_RENDERBUFFER, rb);

  m_rb.push_back(rb);
}

void Framebuffer::blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter)
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  GLenum f = -1;
  switch(filter) {
  case Sampler::Nearest: f = GL_NEAREST;
  case Sampler::Linear:  f = GL_LINEAR;
  }

  glBlitFramebuffer(src.x, src.y, src.z, src.w, dst.x, dst.y, dst.z, dst.w, mask, f);

  glBindFramebuffer(m_bound, m);
}

void Framebuffer::bind_window(BindTarget target)
{
  glBindFramebuffer(binding(target), 0);
}

GLenum Framebuffer::binding(BindTarget t)
{
  switch(t) {
  case BindTarget::Read: return GL_READ_FRAMEBUFFER;
  case BindTarget::Draw: return GL_DRAW_FRAMEBUFFER;
  case BindTarget::Framebuffer: return GL_FRAMEBUFFER;
  }

  return 0;
}

GLenum Framebuffer::attachement(Attachment att)
{
  if(att >= Color0) return GL_COLOR_ATTACHMENT0 + (att-Color0);
  else if(att == Depth) return GL_DEPTH_ATTACHMENT;
  else if(att == DepthStencil) return GL_DEPTH_STENCIL_ATTACHMENT;

  return 0;
}

GLenum Framebuffer::internalformat(Format fmt)
{
  static const GLenum table[] = {
    GL_RGB5_A1, GL_RGBA8,
    GL_DEPTH_COMPONENT16,
    GL_DEPTH_COMPONENT24,
    GL_DEPTH24_STENCIL8,
  };
  return table[fmt];
}

void Framebuffer::checkIfBound()
{
  GLint bound = -1;
  switch(m_bound) {
  case GL_READ_FRAMEBUFFER: glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &bound);
  default:                  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
  }

  if(bound != m) throw "INVALID OPERATION!";
}

ivec2 Framebuffer::getColorAttachementDimensions()
{
  GLint att_type = -1;
  glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &att_type);

  GLint name = -1;
  glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

  ivec2 dims{ -1, -1 };
  switch(att_type) {
  case GL_NONE: throw "INVALID OPERATION! No color attachement";

  case GL_RENDERBUFFER:
    glBindRenderbuffer(GL_RENDERBUFFER, name);

    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &dims.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &dims.y);
    break;

  case GL_TEXTURE: {
    glBindTexture(GL_TEXTURE_2D, name);

    int level = -1;
    glGetFramebufferAttachmentParameteriv(m_bound, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, level , GL_TEXTURE_WIDTH, &dims.x);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &dims.y);
    break;
  }

  default: break;
  }
  return dims;
}

void clear(int mask)
{
  glClear(mask);
}

}