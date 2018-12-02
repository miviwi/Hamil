#pragma once

#include <ek/euklid.h>

#include <util/smallvector.h>
#include <util/hash.h>
#include <math/geometry.h>

#include <optional>
#include <atomic>

namespace gx {
enum Format;

class ResourcePool;
class TextureHandle;
}

namespace ek {

class Renderer;

class RenderTargetConfig {
public:
  enum BaseType : u32 {
    DepthPrepass,
    Forward, Deffered,
    LightPropagationVolume,
    ShadowMap,
    ReflectiveShadowMap,
  };

  BaseType type;

  ivec4 viewport;
  u32 samples = 0;

  std::optional<u32 /* gx::Format */> accumulation = std::nullopt;
  std::optional<u32 /* gx::Format */> linearz = std::nullopt;
  u32 depth = 0;

  // 0 == no MSAA
  static RenderTargetConfig depth_prepass(uint samples = 0);

  // 0 == no MSAA
  //   - HDR internal format (rgb16f)
  static RenderTargetConfig forward_linearz(uint samples = 0);

  struct Hash {
    size_t operator()(const RenderTargetConfig& c) const
    {
      using U32Hash = std::hash<u32>;
      using OptU32Hash = std::hash<std::optional<u32>>;

      size_t hash;

      util::hash_combine<U32Hash>(hash, c.type);
      util::hash_combine<U32Hash>(hash, c.samples);
      util::hash_combine<OptU32Hash>(hash, c.accumulation);
      util::hash_combine<OptU32Hash>(hash, c.linearz);
      util::hash_combine<U32Hash>(hash, c.depth);

      return hash;
    }
  };
};

class RenderTarget {
public:
  RenderTarget(const RenderTarget& other);

  // Create a new RenderTarget accorind to 'config'
  static RenderTarget from_config(const RenderTargetConfig& config, gx::ResourcePool& pool);

  // Returns the RenderTargetConfig used to create this RenderTarget
  const RenderTargetConfig& config() const;
  // Returns a gx::ResourcePool::Id which refers to this RenderTarget's
  //   gx::Framebuffer
  u32 framebufferId() const;

  // Returns 'true' when 'other' was created
  //   with the same config
  bool operator==(const RenderTarget& other) const;

  struct Hash {
    size_t operator()(const RenderTarget& rt) const
    {
      RenderTargetConfig::Hash hash;

      return hash(rt.config());
    }
  };

private:
  friend Renderer;

  using TextureIds = util::SmallVector<u32, sizeof(u32) * /* Minimum required MRTs */ 8>;

  RenderTarget(const RenderTargetConfig& config);

  gx::TextureHandle createTexMultisample(gx::ResourcePool& pool, gx::Format fmt, uint samples);
  gx::TextureHandle createTex(gx::ResourcePool& pool, gx::Format fmt);

  void initForward(gx::ResourcePool& pool);
  void initDepthPrepass(gx::ResourcePool& pool);

  bool lock() const;
  void unlock() const;

  RenderTargetConfig m_config;

  u32 m_fb_id;
  TextureIds m_texture_ids;

  mutable std::atomic<bool> m_in_use;
};

}