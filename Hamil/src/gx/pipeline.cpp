#include <gx/pipeline.h>

#include <cassert>
#include <cstring>
#include <algorithm>

namespace gx {

Pipeline p_current;

Pipeline::Pipeline()
{
}

const Pipeline& Pipeline::use() const
{
  STUB();

  return *this;
}

Pipeline& Pipeline::viewport(int x, int y, int w, int h)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::scissor(int x, int y, int w, int h)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::scissor(ivec4 rect)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::noScissor()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::noBlend()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::noDepthTest()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::noCull()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::filledPolys()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::currentScissor()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::seamlessCubemap()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::noSeamlessCubemap()
{
  STUB();
  
  return *this;
}

Pipeline Pipeline::current()
{
  return p_current;
}

bool Pipeline::isEnabled(unsigned what)
{
  STUB();

  return false;
}

Pipeline& Pipeline::alphaBlend()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::premultAlphaBlend()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::additiveBlend()
{
  STUB();
  
  return *this;
}

Pipeline & Pipeline::subtractiveBlend()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::multiplyBlend()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::blendColor(vec4 color)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::depthTest(CompareFunc func)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::cull(FrontFace front, CullMode mode)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::cull(CullMode mode)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::clearColor(vec4 color)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::clearDepth(float depth)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::clearStencil(int stencil)
{ 
  STUB();
  
  return *this;
}

Pipeline& Pipeline::clear(vec4 color, float depth)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::wireframe()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::primitiveRestart(unsigned index)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::writeDepthOnly()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::depthStencilMask(bool depth, uint stencil)
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::writeColorOnly()
{
  STUB();
  
  return *this;
}

Pipeline& Pipeline::colorMask(bool red, bool green, bool blue, bool alpha)
{
  STUB();
  
  return *this;
}

void Pipeline::disable(unsigned config) const
{
  STUB();
}

void Pipeline::enable(unsigned config) const
{
  STUB();
}

bool Pipeline::compare(const unsigned config) const
{
  STUB();
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
