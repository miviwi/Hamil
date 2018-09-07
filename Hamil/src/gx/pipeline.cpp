#include <gx/pipeline.h>

#include <cassert>
#include <cstring>
#include <algorithm>

namespace gx {

Pipeline p_current;

Pipeline::Pipeline()
{
  memset(this, 0xFF, sizeof(*this));
  std::fill(m_enabled, m_enabled+NumConfigTypes, false);

  m_viewport = { -1, -1, -1, -1 };
  m_scissor.current = false;
  m_depth.func = GL_LESS;
  m_cull.front = GL_CCW;
  m_clear.stencil = ~0;
  m_restart.index = 0;

  m_enabled[Mask] = true;
  m_mask.red = m_mask.green = m_mask.blue = m_mask.alpha = true;
  m_mask.depth   = true;
  m_mask.stencil = ~0;

  m_enabled[Cubemap] = true;
}

void Pipeline::use() const
{
  for(unsigned i = 0; i < NumConfigTypes; i++) {
    auto config = (ConfigType)i;

    if(!m_enabled[i] && !p_current.m_enabled[i]) {
      continue;
    } else if(!m_enabled[i]) {
      disable(config);
    } else if(!compare(config)) {
      enable(config);
    }
  }

  p_current = *this;
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
  s.current = false;
  s.x = x; s.y = y; s.width = w; s.height = h;

  return *this;
}

Pipeline& Pipeline::scissor(ivec4 rect)
{
  return scissor(rect.x, rect.y, rect.z, rect.w);
}

Pipeline& Pipeline::noScissor()
{
  m_enabled[Scissor] = false;

  return *this;
}

Pipeline& Pipeline::noBlend()
{
  m_enabled[Blend] = false;

  return *this;
}

Pipeline& Pipeline::noDepthTest()
{
  m_enabled[Depth] = false;

  return *this;
}

Pipeline& Pipeline::noCull()
{
  m_enabled[Cull] = false;

  return *this;
}

Pipeline& Pipeline::filledPolys()
{
  m_enabled[Wireframe] = false;

  return *this;
}

Pipeline& Pipeline::currentScissor()
{
  m_enabled[Scissor] = true;
  m_scissor.current = true;

  return *this;
}

Pipeline& Pipeline::seamlessCubemap()
{
  m_enabled[Cubemap] = true;
  m_cubemap.seamless = true;

  return *this;
}

Pipeline& Pipeline::noSeamlessCubemap()
{
  m_enabled[Cubemap] = true;
  m_cubemap.seamless = false;

  return *this;
}

Pipeline Pipeline::current()
{
  return p_current;
}

bool Pipeline::isEnabled(ConfigType what)
{
  return m_enabled[what];
}

Pipeline& Pipeline::alphaBlend()
{
  m_enabled[Blend] = true;
  m_blend.mode = GL_FUNC_ADD;
  m_blend.sfactor = GL_SRC_ALPHA; m_blend.dfactor = GL_ONE_MINUS_SRC_ALPHA;

  return *this;
}

Pipeline& Pipeline::additiveBlend()
{
  m_enabled[Blend] = true;
  m_blend.mode = GL_FUNC_ADD;
  m_blend.sfactor = GL_ONE; m_blend.dfactor = GL_ONE;

  return *this;
}

Pipeline & Pipeline::subtractiveBlend()
{
  m_enabled[Blend] = true;
  m_blend.mode = GL_FUNC_SUBTRACT;
  m_blend.sfactor = GL_ONE; m_blend.dfactor = GL_ONE;

  return *this;
}

Pipeline& Pipeline::multiplyBlend()
{
  m_enabled[Blend] = true;
  m_blend.mode = GL_FUNC_ADD;
  m_blend.sfactor = GL_DST_COLOR; m_blend.dfactor = GL_ZERO;

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

Pipeline& Pipeline::cull(CullMode mode)
{
  return cull(CounterClockwise, mode);
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

Pipeline& Pipeline::wireframe()
{
  m_enabled[Wireframe] = true;

  return *this;
}

Pipeline& Pipeline::primitiveRestart(unsigned index)
{
  m_enabled[PrimitiveRestart] = true;
  m_restart.index = index;

  return *this;
}

Pipeline& Pipeline::writeDepthOnly()
{
  m_enabled[Mask] = true;

  m_mask.red = m_mask.green = m_mask.blue = m_mask.alpha = false;

  m_mask.depth   = true;
  m_mask.stencil = 0;

  return *this;
}

Pipeline& Pipeline::depthStencilMask(bool depth, uint stencil)
{
  m_enabled[Mask] = true;

  m_mask.depth   = depth;
  m_mask.stencil = stencil;

  return *this;
}

Pipeline& Pipeline::writeColorOnly()
{
  m_enabled[Mask] = true;

  m_mask.red = m_mask.green = m_mask.blue = m_mask.alpha = true;

  m_mask.depth   = false;
  m_mask.stencil = 0;

  return *this;
}

Pipeline& Pipeline::colorMask(bool red, bool green, bool blue, bool alpha)
{
  m_enabled[Mask] = true;

  m_mask.red = red; m_mask.green = green; m_mask.blue = blue; m_mask.alpha = alpha;

  return *this;
}

void Pipeline::disable(ConfigType config) const
{
  switch(config) {
  case Viewport:  break;
  case Scissor:   glDisable(GL_SCISSOR_TEST); break;
  case Blend:     glDisable(GL_BLEND); break;
  case Depth:     glDisable(GL_DEPTH_TEST); break;
  case Stencil:   glDisable(GL_STENCIL_TEST); break;
  case Cull:      glDisable(GL_CULL_FACE); break;
  case Clear:     break;
  case Wireframe: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
  case PrimitiveRestart: glDisable(GL_PRIMITIVE_RESTART); break;
  case Mask:      break;
  case Cubemap:   break;

  default: break;
  }
}

void Pipeline::enable(ConfigType config) const
{
  auto do_enable = [](ConfigType config, GLenum cap)
  {
    if(!p_current.isEnabled(config)) glEnable(cap);
  };

  const auto& v  = m_viewport;
  const auto& sc = m_scissor;
  const auto& c  = m_clear;
  const auto& m  = m_mask;

  switch(config) {
  case Viewport: glViewport(v.x, v.y, v.width, v.height); break;
  case Scissor:
    if(!sc.current) {
      do_enable(Scissor, GL_SCISSOR_TEST);
      glScissor(sc.x, sc.y, sc.width, sc.height);
    }
    break;
  case Blend:
    do_enable(Blend, GL_BLEND);
    glBlendEquation(m_blend.mode);
    glBlendFunc(m_blend.sfactor, m_blend.dfactor);
    break;
  case Depth:
    do_enable(Depth, GL_DEPTH_TEST);
    if(p_current.m_depth.func != m_depth.func) glDepthFunc(m_depth.func);
    break;
  case Stencil:
    assert(0 && "StencilConfig unimpleneted!");
    // TODO
    break;
  case Cull:
    do_enable(Cull, GL_CULL_FACE);
    glFrontFace(m_cull.front);
    glCullFace(m_cull.mode);
    break;
  case Clear:
    glClearColor(c.color.r, c.color.g, c.color.b, c.color.a);
    glClearDepth(c.depth);
    glClearStencil(c.stencil);
    break;
  case Wireframe: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
  case PrimitiveRestart:
    do_enable(PrimitiveRestart, GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(m_restart.index);
    break;
  case Mask:
    glColorMask(m.red, m.green, m.blue, m.alpha);
    glDepthMask(m.depth);
    glStencilMask(m.stencil);
    break;
  case Cubemap:
    m_cubemap.seamless ? glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS) : glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    break;

  default: break;
  }
}

bool Pipeline::compare(const ConfigType config) const
{
  auto do_compare = [](const auto& a, const auto& b) -> bool
  {
    return !memcmp(&a, &b, sizeof(a));
  };

  switch(config) {
  case Viewport:         return do_compare(m_viewport, p_current.m_viewport);
  case Scissor:          return do_compare(m_scissor, p_current.m_scissor);
  case Blend:            return do_compare(m_blend, p_current.m_blend);
  case Depth:            return do_compare(m_depth, p_current.m_depth);
  case Stencil:          break;  //return do_compare(m_stencil, p_current.m_stencil);
  case Cull:             return do_compare(m_cull, p_current.m_cull);
  case Clear:            return do_compare(m_clear, p_current.m_clear);
  case Wireframe:        return false;
  case PrimitiveRestart: return do_compare(m_restart, p_current.m_restart);
  case Mask:             return do_compare(m_mask, p_current.m_mask);
  case Cubemap:          return do_compare(m_cubemap, p_current.m_cubemap);
  }

  return false;
}

ScopedPipeline::ScopedPipeline(const Pipeline& p)
{
  m = Pipeline::current();

  p.use();
}

ScopedPipeline::~ScopedPipeline()
{
  m.use();
}

}
