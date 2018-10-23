#pragma once

#include <gx/gx.h>
#include <gx/pipeline.h>
#include <gx/resourcepool.h>

#include <util/ref.h>

#include <array>
#include <tuple>
#include <initializer_list>
#include <utility>

namespace gx {

class Framebuffer;
class Texture;
class Sampler;

class RenderPass : public Ref {
public:
  enum ClearOp : unsigned {
    NoClear,
    ClearColor   = (1<<0),
    ClearDepth   = (1<<1),
    ClearStencil = (1<<2),

    ClearColorDepth = ClearColor|ClearDepth,

    ClearAll = ~0u,
  };

  using TextureAndSampler = std::tuple<ResourcePool::Id /* texture */, ResourcePool::Id /* sampler */>;

  RenderPass();
  ~RenderPass();

  RenderPass& framebuffer(ResourcePool::Id fb);
  RenderPass& texture(unsigned unit, ResourcePool::Id tex, ResourcePool::Id sampler);
  RenderPass& textures(std::initializer_list<std::pair<unsigned, TextureAndSampler>> tus);
  RenderPass& pipeline(const Pipeline& p);
  RenderPass& clearOp(unsigned op);

  const RenderPass& begin(ResourcePool& pool) const;

private:
  ResourcePool::Id m_framebuffer;
  std::array<TextureAndSampler, NumTexUnits> m_texunits;
  Pipeline m_pipeline;

  unsigned m_clear;
};

}