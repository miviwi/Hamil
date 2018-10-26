#pragma once

#include <gx/gx.h>
#include <gx/pipeline.h>

#include <util/ref.h>
#include <util/smallvector.h>

#include <array>
#include <vector>
#include <tuple>
#include <optional>
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

  enum : size_t {
    RangeNone = ~0ull,
  };

  using TextureAndSampler = std::tuple<ResourceId /* texture */, ResourceId /* sampler */>;

  template <typename T>
  using PairInitList = std::initializer_list<std::pair<unsigned, T>>;

  struct RangedResource {
    ResourceId buf;
    size_t offset, size;
  };

  class Subpass {
  public:
    Subpass();

    Subpass& pipeline(const Pipeline& pipeline);
    Subpass& texture(unsigned unit, ResourceId texture, ResourceId sampler);
    Subpass& uniformBuffer(unsigned index, ResourceId buf);
    Subpass& uniformBufferRange(unsigned index, ResourceId buf, size_t offset, size_t size);

    const Subpass& use(ResourcePool& pool) const;

  private:
    std::optional<Pipeline> m_pipeline;
    util::SmallVector<std::pair<unsigned, TextureAndSampler>> m_texunits;
    util::SmallVector<std::pair<unsigned, RangedResource>, 64> m_uniform_bufs;
  };

  RenderPass();
  ~RenderPass();

  RenderPass& framebuffer(ResourceId fb);
  RenderPass& texture(unsigned unit, ResourceId tex, ResourceId sampler);
  RenderPass& textures(PairInitList<TextureAndSampler> tus);
  RenderPass& uniformBuffer(unsigned index, ResourceId buf);
  RenderPass& uniformBuffers(PairInitList<ResourceId /* buf */> bufs);
  RenderPass& uniformBufferRange(unsigned index, ResourceId buf, size_t offset, size_t size);
  RenderPass& uniformBuffersRange(PairInitList<RangedResource> bufs);
  RenderPass& pipeline(const Pipeline& p);
  RenderPass& clearOp(unsigned op);

  // The 'id's for beginSubpass() are assigned sequentially
  //   - nextSubpassId() can be used to query the next one
  RenderPass& subpass(const Subpass& subpass);

  // Returns the 'id' which will be assigned to the next added subpass()
  uint nextSubpassId() const;

  const RenderPass& begin(ResourcePool& pool) const;
  const RenderPass& beginSubpass(ResourcePool& pool, uint id) const;

private:
  ResourceId m_framebuffer;
  std::array<TextureAndSampler, NumTexUnits> m_texunits;
  std::array<RangedResource /* UniformBuffer */, NumUniformBindings> m_uniform_bufs;
  Pipeline m_pipeline;

  std::vector<Subpass> m_subpasses;

  unsigned m_clear;
};

}