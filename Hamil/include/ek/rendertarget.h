#pragma once

#include <ek/euklid.h>

#include <util/ref.h>
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

  // Returns 'true' when 'other' is compatible
  bool operator==(const RenderTargetConfig& other) const;

};

class RenderTarget : public Ref {
public:
  struct Error { };

  struct CreateError : public Error { };

  RenderTarget(const RenderTarget& other);
  ~RenderTarget();

  // Create a new RenderTarget accorind to 'config'
  static RenderTarget from_config(const RenderTargetConfig& config, gx::ResourcePool& pool);

  // Returns the RenderTargetConfig used to create this RenderTarget
  const RenderTargetConfig& config() const;
  // Returns a gx::ResourcePool::Id which refers to this RenderTarget's
  //   gx::Framebuffer
  u32 framebufferId() const;

private:
  friend Renderer;

  using TextureIds = util::SmallVector<u32, sizeof(u32) * /* Minimum required MRTs */ 8>;

  RenderTarget(const RenderTargetConfig& config);

  gx::TextureHandle createTexMultisample(gx::ResourcePool& pool, gx::Format fmt, uint samples);
  gx::TextureHandle createTex(gx::ResourcePool& pool, gx::Format fmt);

  void initForward(gx::ResourcePool& pool);
  void initDepthPrepass(gx::ResourcePool& pool);

  void checkComplete(gx::ResourcePool& pool);

  bool lock() const;
  void unlock() const;

  RenderTargetConfig m_config;

  u32 m_fb_id;
  TextureIds m_texture_ids;

  mutable std::atomic<bool> m_in_use;
};

}