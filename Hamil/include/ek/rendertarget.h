#pragma once

#include <ek/euklid.h>
#include <ek/sharedobject.h>

#include <util/ref.h>
#include <util/smallvector.h>
#include <util/hash.h>
#include <math/geometry.h>

#include <optional>
#include <atomic>

namespace gx {
enum Format : uint;
enum Type : uint;

class ResourcePool;
class TextureHandle;
class Framebuffer;
}

namespace ek {

class Renderer;

class RenderTargetConfig {
public:
  enum BaseType : u32 {
    DepthPrepass,
    Forward, Deferred,
    LightPropagationVolume,
    ShadowMap,
    ReflectiveShadowMap,
  };

  BaseType type;

  ivec4 viewport;
  u32 samples = 0;

  std::optional<u32 /* gx::Format */> accumulation = std::nullopt;
  std::optional<u32 /* gx::Format */> linearz = std::nullopt;

  std::optional<u32 /* gx::Format */> moments = std::nullopt;

  bool depth_texture = true;
  u32 depth = 0;

  // 0 == no MSAA
  static RenderTargetConfig depth_prepass(uint samples = 0);

  // 0 == no MSAA
  //   - HDR internal format (rgb16f)
  static RenderTargetConfig forward_linearz(uint samples = 0);

  // 0 == no MSAA
  static RenderTargetConfig moment_shadow_map(uint samples = 0);

  // Returns 'true' when 'other' is compatible
  bool operator==(const RenderTargetConfig& other) const;
};

class RenderTarget : public SharedObject, public Ref {
public:
  enum TextureType {
    // Forward
    Accumulation = 0,
    LinearZ = 1,

    // MomentShadowMap
    Moments = 0,

    // Depth texture for all BaseTypes
    Depth = ~0,
  };

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

  u32 textureId(TextureType type) const;

private:
  friend Renderer;

  using TextureIds = util::SmallVector<u32, /* gx::NumMRTBindings */ 8*sizeof(u32)>;

  RenderTarget(const RenderTargetConfig& config, gx::ResourcePool& pool);

  std::string labelSuffix() const;

  // Stores the Id in 'm_fb_id'
  gx::Framebuffer& createFramebuffer();

  gx::TextureHandle createTexMultisample(u32 /* gx::Format */ fmt, uint samples,
    const std::string& label);
  gx::TextureHandle createTex(u32 /* gx::Format */ fmt,
    const std::string& label);

  // Initialize the depth buffer
  //   - Either as a Texture or Framebuffer::renderbuffer()
  //     depending on m_config.depth_texture
  void initDepth();

  // m_config.type == Forward
  void initForward();
  // m_config.type == DepthPrepass
  void initDepthPrepass();
  // m_config.type == ShadowMap
  void initShadowMap();

  // Check Framebuffer completeness
  void checkComplete();

  gx::Framebuffer& getFramebuffer();

  // m_config.type == Forward
  u32 forwardTextureId(TextureType type) const;
  // m_config.type == ShadowMap
  u32 shadowMapTexureId(TextureType type) const;

  RenderTargetConfig m_config;

  gx::ResourcePool *m_pool;

  u32 m_fb_id;
  TextureIds m_texture_ids;
};

}