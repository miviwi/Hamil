#pragma once

#include <ek/euklid.h>

#include <math/geometry.h>
#include <math/frustum.h>

#include <array>
#include <vector>

namespace gx {
class Pipeline;
class CommandBuffer;
class ResourcePool;
class MemoryPool;
}

namespace ek {

class Renderer;
class RenderObject;

// Stores a MemoryPool Handle and a size
struct ShaderConstants;

class RenderView {
public:
  enum ViewType {
    Invalid,

    // Fully lit and postprocessed view
    CameraView,

    // Reflective shadow map
    LightView,

    // Shadow map
    ShadowView,
  };

  enum RenderType {
    // Used for depth pre-pass
    DepthOnly,

    // Forward rendeing fallback pieline
    Forward,

    // Deferred pipeline
    Deferred,
  };

  RenderView(ViewType type);

  // RenderType = DepthOnly
  RenderView& depthPrepass();
  // RenderType = Forward
  RenderView& forwardRender();
  // RenderType = Deferred
  RenderView& deferredRender();

  // Sets the viewport to
  //      (x=0, y=0, width=viewport.x, height=viewport.y)
  //  and determines the Framebuffer's resolution
  RenderView& viewport(ivec2 viewport);

  // Sets the MSAA sample count
  //   - MSAA is disabled by default
  RenderView& sampleCount(uint samples);

  RenderView& view(const mat4& v);
  RenderView& projection(const mat4& p);

  // Used by Renderer::extractForView()
  frustum3 constructFrustum();

  // Make SURE to specify the ResourcePool used to create
  //   the hm::Mesh Components of the extracted Entities!
  // Because MemoryPool has no thread safety each concurrent
  //   call of render() must have an independent one
  gx::CommandBuffer render(Renderer& renderer,
    const std::vector<RenderObject>& objects,
    gx::MemoryPool *mempool, gx::ResourcePool *pool);

private:
  gx::Pipeline createPipeline();
  u32 createFramebuffer();
  u32 createRenderPass();

  u32 constantBlockSizeAlign(u32 sz);

  ShaderConstants generateSceneConstants();
  ShaderConstants generateConstants(const RenderObject& ro);

  u32 getProgram(const RenderObject& ro);

  void renderOne(const RenderObject& ro, gx::CommandBuffer& cmd);

  ViewType m_type;
  RenderType m_render;

  u32 m_ubo_alignment;

  // x, y, width, height
  ivec4 m_viewport;

  // 0 == no MSAA
  uint m_samples;

  mat4 m_view;
  mat4 m_projection;

  Renderer *m_renderer;

  gx::MemoryPool *m_mempool;
  gx::ResourcePool *m_pool;

  // Stores the number of currently used render tagrets
  uint m_num_rts;
  // Stores the render target textures
  std::array<u32, 8 /* Minimum required render targets */> m_rts;

  // Stores ids of Programs
  std::vector<u32> m_programs;

  // TODO: allocate this somewhere
  u32 m_scene_ubo_id;
  // TODO: allocate this somewhere
  u32 m_object_ubo_id;
};

}