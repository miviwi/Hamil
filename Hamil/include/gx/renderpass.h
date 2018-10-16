#pragma once

#include <gx/gx.h>
#include <gx/pipeline.h>

#include <array>
#include <tuple>
#include <initializer_list>
#include <utility>

namespace gx {

class Framebuffer;
class Texture;
class Sampler;

// The RenderPass doesn't own any of it's Framebuffers, Textures, Samplers etc.
//   which means they MUST be alive before calling begin() or undefined behaviour
//   will occur
class RenderPass {
public:
  enum ClearOp : unsigned {
    NoClear,
    ClearColor   = (1<<0),
    ClearDepth   = (1<<1),
    ClearStencil = (1<<2),

    ClearColorDepth = ClearColor|ClearDepth,

    ClearAll = ~0u,
  };

  using TextureAndSampler = std::tuple<Texture *, Sampler *>;

  RenderPass();

  RenderPass& framebuffer(Framebuffer& fb);
  RenderPass& texture(unsigned unit, Texture& tex, Sampler& sampler);
  RenderPass& textures(std::initializer_list<std::pair<unsigned /* unit */, TextureAndSampler>> tus);
  RenderPass& pipeline(const Pipeline& p);
  RenderPass& clearOp(unsigned op);

  const RenderPass& begin() const;

private:
  Framebuffer *m_framebuffer;
  std::array<TextureAndSampler, NumTexUnits> m_texunits;
  Pipeline m_pipeline;

  unsigned m_clear;
};

}