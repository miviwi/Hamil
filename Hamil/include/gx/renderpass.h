#pragma once

#include <gx/gx.h>
#include <gx/pipeline.h>

#include <util/ref.h>

#include <array>
#include <tuple>
#include <initializer_list>
#include <utility>

namespace gx {

class Framebuffer;
class Texture;
class Sampler;
class ResourcePool;

class RenderPass : public Ref {
public:
  using ResourceId = ::u32;

  enum ClearOp : unsigned {
    NoClear,
    ClearColor   = (1<<0),
    ClearDepth   = (1<<1),
    ClearStencil = (1<<2),

    ClearColorDepth = ClearColor|ClearDepth,

    ClearAll = ~0u,
  };

  using TextureAndSampler = std::tuple<ResourceId /* texture */, ResourceId /* sampler */>;

  template <typename T>
  using PairInitList = std::initializer_list<std::pair<unsigned, T>>;

  RenderPass();
  ~RenderPass();

  RenderPass& framebuffer(ResourceId fb);
  RenderPass& texture(unsigned unit, ResourceId tex, ResourceId sampler);
  RenderPass& textures(PairInitList<TextureAndSampler> tus);
  RenderPass& uniformBuffer(unsigned index, ResourceId buf);
  RenderPass& uniformBuffers(PairInitList<ResourceId /* buf */> bufs);
  RenderPass& pipeline(const Pipeline& p);
  RenderPass& clearOp(unsigned op);

  const RenderPass& begin(ResourcePool& pool) const;

private:
  ResourceId m_framebuffer;
  std::array<TextureAndSampler, NumTexUnits> m_texunits;
  std::array<ResourceId /* UniformBuffer */, NumUniformBindings> m_uniform_bufs;
  Pipeline m_pipeline;

  unsigned m_clear;
};

}