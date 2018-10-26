#include <gx/renderpass.h>
#include <gx/framebuffer.h>
#include <gx/texture.h>
#include <gx/pipeline.h>
#include <gx/program.h>
#include <gx/resourcepool.h>

namespace gx {

RenderPass::RenderPass() :
  m_clear(NoClear)
{
  for(auto& tu : m_texunits) tu = { ResourcePool::Invalid, ResourcePool::Invalid };
  for(auto& buf : m_uniform_bufs) buf = { ResourcePool::Invalid, RangeNone, RangeNone };
}

RenderPass::~RenderPass()
{
  if(deref()) return;
}

RenderPass& RenderPass::framebuffer(ResourceId fb)
{
  m_framebuffer = fb;

  return *this;
}

RenderPass& RenderPass::texture(unsigned unit, ResourceId tex, ResourceId sampler)
{
  m_texunits[unit] = { tex, sampler };

  return *this;
}

RenderPass& RenderPass::textures(PairInitList<TextureAndSampler> tus)
{
  for(const auto& tu : tus) {
    auto tex     = std::get<0>(tu.second);
    auto sampler = std::get<1>(tu.second);

    texture(tu.first, tex, sampler);
  }

  return *this;
}

RenderPass& RenderPass::uniformBuffer(unsigned index, ResourceId buf)
{
  m_uniform_bufs[index] = { buf, RangeNone, RangeNone };

  return *this;
}

RenderPass& RenderPass::uniformBuffers(PairInitList<ResourceId> bufs)
{
  for(const auto& buf : bufs) {
    uniformBuffer(buf.first, buf.second);
  }

  return *this;
}

RenderPass& RenderPass::uniformBufferRange(unsigned index, ResourceId buf, size_t offset, size_t size)
{
  m_uniform_bufs[index] = { buf, offset, size };

  return *this;
}

RenderPass& RenderPass::uniformBuffersRange(PairInitList<RangedResource> bufs)
{
  for(const auto& buf : bufs) {
    uniformBufferRange(buf.first, buf.second.buf, buf.second.offset, buf.second.size);
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

RenderPass& RenderPass::subpass(const Subpass& subpass)
{
  m_subpasses.emplace_back(subpass);

  return *this;
}

uint RenderPass::nextSubpassId() const
{
  return (uint)m_subpasses.size();
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

  for(unsigned bufi = 0; bufi < (unsigned)m_uniform_bufs.size(); bufi++) {
    const auto& buf_range = m_uniform_bufs[bufi];
    if(buf_range.buf == ResourcePool::Invalid) continue;

    auto& buf = pool.getBuffer<UniformBuffer>(buf_range.buf);
    if(buf_range.offset != RangeNone && buf_range.size != RangeNone) {
      buf.bindToIndex(bufi, buf_range.offset, buf_range.size);
    } else {
      buf.bindToIndex(bufi);
    }
  }

  if(m_clear) framebuffer.clear(m_clear);

  return *this;
}

const RenderPass& RenderPass::beginSubpass(ResourcePool& pool, uint id) const
{
  assert(id < m_subpasses.size() && "subpass 'id' out of range!");

  m_subpasses[id].use(pool);

  return *this;
}

RenderPass::Subpass::Subpass() :
  m_pipeline(std::nullopt)
{
}

RenderPass::Subpass& RenderPass::Subpass::pipeline(const Pipeline & pipeline)
{
  m_pipeline = pipeline;

  return *this;
}

RenderPass::Subpass& RenderPass::Subpass::texture(unsigned unit, ResourceId tex, ResourceId sampler)
{
  m_texunits.emplace(unit, std::make_tuple(tex, sampler));

  return *this;
}

RenderPass::Subpass& RenderPass::Subpass::uniformBuffer(unsigned index, ResourceId buf)
{
  m_uniform_bufs.emplace(index, RangedResource{ buf, RangeNone, RangeNone });

  return *this;
}

RenderPass::Subpass& RenderPass::Subpass::uniformBufferRange(unsigned index,
  ResourceId buf, size_t offset, size_t size)
{
  m_uniform_bufs.emplace(index, RangedResource{ buf, offset, size });

  return *this;
}

const RenderPass::Subpass& RenderPass::Subpass::use(ResourcePool& pool) const
{
  if(m_pipeline) {
    m_pipeline->use();
  }

  for(auto it = m_texunits.cbegin(); it != m_texunits.cend(); it++) {
    const auto& tu = *it;

    auto tex     = pool.getTexture(std::get<0>(tu.second));
    auto sampler = pool.get<Sampler>(std::get<1>(tu.second));
    gx::tex_unit(tu.first, tex(), sampler);
  }

  for(auto it = m_uniform_bufs.cbegin(); it != m_uniform_bufs.cend(); it++) {
    const auto& buf = *it;
    auto buf_range = buf.second;

    auto buffer = pool.getBuffer<UniformBuffer>(buf_range.buf);
    if(buf_range.offset != RangeNone && buf_range.size != RangeNone) {
      buffer.bindToIndex(buf.first, buf_range.offset, buf_range.size);
    } else {
      buffer.bindToIndex(buf.first);
    }
  }

  return *this;
}

}