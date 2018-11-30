#pragma once

#include <ek/euklid.h>

#include <util/smallvector.h>
#include <math/geometry.h>

namespace gx {
enum Format;

class ResourcePool;
class TextureHandle;
}

namespace ek {

class RenderTarget {
public:
  // 0 == no MSAA
  static RenderTarget createForwardWithLinearZ(gx::ResourcePool& pool,
    ivec4 viewport, uint samples = 0);

  // 0 == no MSAA
  static RenderTarget createDepthPrepass(gx::ResourcePool& pool,
    ivec4 viewport, uint samples = 0);

private:
  using TextureIds = util::SmallVector<u32, sizeof(u32) * /* Minimum required MRTs */ 8>;

  RenderTarget(ivec4 viewport);

  gx::TextureHandle createTexMultisample(gx::ResourcePool& pool, gx::Format fmt, uint samples);
  gx::TextureHandle createTex(gx::ResourcePool& pool, gx::Format fmt);

  ivec4 m_viewport;
  u32 m_fb_id;
  TextureIds m_texture_ids;
};

}