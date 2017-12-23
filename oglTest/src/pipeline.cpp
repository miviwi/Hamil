#include "pipeline.h"

#include <cassert>
#include <algorithm>

namespace gx {

Pipeline::Pipeline()
{
  std::fill(m_enabled, m_enabled+NumConfigTypes, false);

  m_enabled[Scissor] = true;
  m_scissor.set = false;
}

void Pipeline::use() const
{
  for(unsigned i = 0; i < NumConfigTypes; i++) {
    if(!m_enabled[i]) disable((ConfigType)i);
    else              enable((ConfigType)i);
  }
}

Pipeline& Pipeline::viewport(int x, int y, int w, int h)
{
  auto& v = m_viewport;

  m_enabled[Viewport] = true;
  v.x = x; v.y = y; v.width = w; v.height = h;
  
  return *this;
}

Pipeline& Pipeline::scissor(int x, int y, int w, int h)
{
  auto& s = m_scissor;

  m_enabled[Scissor] = true;
  s.set = true;
  s.x = x; s.y = y; s.width = w; s.height = h;

  return *this;
}

Pipeline& Pipeline::noScissor()
{
  m_enabled[Scissor] = false;

  return *this;
}

Pipeline& Pipeline::alphaBlend()
{
  m_enabled[Blend] = true;
  m_blend.sfactor = GL_SRC_ALPHA; m_blend.dfactor = GL_ONE_MINUS_SRC_ALPHA;

  return *this;
}

Pipeline& Pipeline::depthTest(DepthFunc func)
{
  m_enabled[Depth] = true;
  m_depth.func = (GLenum)func;


  return *this;
}

Pipeline& Pipeline::cull(FrontFace front, CullMode mode)
{
  m_enabled[Cull] = true;
  m_cull.front = (GLenum)front; m_cull.mode = (GLenum)mode;

  return *this;
}

Pipeline& Pipeline::clearColor(vec4 color)
{
  m_enabled[Clear] = true;
  m_clear.color = color;

  return *this;
}

Pipeline& Pipeline::clearDepth(float depth)
{
  m_enabled[Clear] = true;
  m_clear.depth = depth;

  return *this;
}

Pipeline& Pipeline::clearStencil(int stencil)
{ 
  m_enabled[Clear] = true;
  m_clear.stencil = stencil;

  return *this;
}

Pipeline& Pipeline::clear(vec4 color, float depth)
{
  m_enabled[Clear] = true;
  m_clear.color = color; m_clear.depth = depth;

  return *this;
}

void Pipeline::disable(ConfigType config) const
{
  switch(config) {
  case Viewport: break;
  case Scissor:  glDisable(GL_SCISSOR_TEST); break;
  case Blend:    glDisable(GL_BLEND); break;
  case Depth:    glDisable(GL_DEPTH_TEST); break;
  case Stencil:  glDisable(GL_STENCIL_TEST); break;
  case Cull:     glDisable(GL_CULL_FACE); break;
  case Clear:    break;

  default: break;
  }
}

void Pipeline::enable(ConfigType config) const
{
  const auto& v = m_viewport;
  const auto& sc = m_scissor;
  const auto& c = m_clear;

  switch(config) {
  case Viewport: glViewport(v.x, v.y, v.width, v.height); break;
  case Scissor:
    glEnable(GL_SCISSOR_TEST);
    if(sc.set) glScissor(sc.x, sc.y, sc.width, sc.height);
    break;
  case Blend:
    glEnable(GL_BLEND);
    glBlendFunc(m_blend.sfactor, m_blend.dfactor);
    break;
  case Depth:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(m_depth.func);
    break;
  case Stencil:
    assert(0 && "StencilConfig unimpleneted!");
    // TODO
    break;
  case Cull:
    glEnable(GL_CULL_FACE);
    glFrontFace(m_cull.front);
    glCullFace(m_cull.mode);
    break;
  case Clear:
    glClearColor(c.color.r, c.color.g, c.color.b, c.color.a);
    glClearDepth(c.depth);
    glClearStencil(c.stencil);
    break;

  default: break;
  }
}

}
