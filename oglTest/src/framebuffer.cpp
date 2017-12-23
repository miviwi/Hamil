#include "framebuffer.h"

namespace gx {

Framebuffer::Framebuffer()
{
  glGenFramebuffers(1, &m);
}

Framebuffer::~Framebuffer()
{
  glDeleteFramebuffers(1, &m);
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
  GLint bound = -1;
  switch(m_bound) {
  case GL_READ_FRAMEBUFFER: glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &bound);
  default:                  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
  }

  if(bound != m) throw "INVALID OPERATION!";

  glFramebufferTexture(m_bound, attachement(att), tex.m, level);
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
  else if(att == DepthStencil) return GL_DEPTH_STENCIL_ATTACHMENT;

  return 0;
}

void clear(Framebuffer::BlitMask mask)
{
  glClear(mask);
}

}