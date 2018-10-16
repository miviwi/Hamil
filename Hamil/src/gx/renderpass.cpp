#include <gx/renderpass.h>
#include <gx/framebuffer.h>
#include <gx/texture.h>
#include <gx/pipeline.h>

namespace gx {

RenderPass::RenderPass() :
  m_framebuffer(nullptr),
  m_clear(NoClear)
{
  for(auto& tu : m_texunits) tu = { nullptr, nullptr };
}

RenderPass& RenderPass::framebuffer(Framebuffer& fb)
{
  m_framebuffer = &fb;

  return *this;
}

RenderPass& RenderPass::texture(unsigned unit, Texture& tex, Sampler& sampler)
{
  m_texunits[unit] = { &tex, &sampler };

  return *this;
}

RenderPass& RenderPass::textures(std::initializer_list<std::pair<unsigned, TextureAndSampler>> tus)
{
  for(const auto& tu : tus) {
    m_texunits[tu.first] = tu.second;
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

const RenderPass& RenderPass::begin() const
{
  m_framebuffer->use();
  m_pipeline.use();

  for(unsigned tui = 0; tui < (unsigned)m_texunits.size(); tui++) {
    const auto& tu = m_texunits[tui];

    const Texture *tex     = std::get<0>(tu);
    const Sampler *sampler = std::get<1>(tu);

    if(tex && sampler) {
      gx::tex_unit(tui, *tex, *sampler);
    }
  }

  if(m_clear) m_framebuffer->clear(m_clear);

  return *this;
}

}