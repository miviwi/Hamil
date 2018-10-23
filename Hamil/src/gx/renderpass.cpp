#include <gx/renderpass.h>
#include <gx/framebuffer.h>
#include <gx/texture.h>
#include <gx/pipeline.h>
#include <gx/program.h>

namespace gx {

RenderPass::RenderPass() :
  m_clear(NoClear)
{
  for(auto& tu : m_texunits) tu = { ResourcePool::Invalid, ResourcePool::Invalid };
}

RenderPass::~RenderPass()
{
  if(deref()) return;
}

RenderPass& RenderPass::framebuffer(ResourcePool::Id fb)
{
  m_framebuffer = fb;

  return *this;
}

RenderPass& RenderPass::texture(unsigned unit, ResourcePool::Id tex, ResourcePool::Id sampler)
{
  m_texunits[unit] = { tex, sampler };

  return *this;
}

RenderPass& RenderPass::textures(std::initializer_list<std::pair<unsigned, TextureAndSampler>> tus)
{
  for(const auto& tu : tus) {
    auto tex     = std::get<0>(tu.second);
    auto sampler = std::get<1>(tu.second);

    texture(tu.first, tex, sampler);
  }

  return *this;
}

RenderPass& RenderPass::pipeline(const Pipeline& p)
{
  m_pipeline = p;

  return *this;
}

RenderPass& RenderPass::clearOp(unsigned op)
{
  m_clear = 0;

  if(op & ClearColor) m_clear |= gx::Framebuffer::ColorBit;
  if(op & ClearDepth) m_clear |= gx::Framebuffer::DepthBit;
  if(op & ClearStencil) m_clear |= gx::Framebuffer::StencilBit;

  return *this;
}

const RenderPass& RenderPass::begin(ResourcePool& pool) const
{ 
  auto& framebuffer = pool.get<Framebuffer>(m_framebuffer);

  framebuffer.use();
  m_pipeline.use();

  for(unsigned tui = 0; tui < (unsigned)m_texunits.size(); tui++) {
    const auto& tu = m_texunits[tui];

    auto tex_id     = std::get<0>(tu);
    auto sampler_id = std::get<1>(tu);
    if(tex_id == ResourcePool::Invalid || sampler_id == ResourcePool::Invalid) continue;

    auto& tex     = pool.getTexture(tex_id);
    auto& sampler = pool.get<Sampler>(sampler_id);
    gx::tex_unit(tui, tex(), sampler);
  }

  if(m_clear) framebuffer.clear(m_clear);

  return *this;
}

}