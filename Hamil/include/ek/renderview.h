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
class RenderTarget;

// Stores a MemoryPool Handle and a size
struct ShaderConstants;

struct ObjectConstants;

// Must keep the RenderView around until render() finishes
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

  static constexpr size_t MempoolInitialAlloc = 4096;

  RenderView(ViewType type);
  ~RenderView();

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
  gx::CommandBuffer render(Renderer& renderer,
    const std::vector<RenderObject>& objects,
    gx::ResourcePool *pool);

private:
  enum {
    SceneConstantsBinding  = 0,
    ObjectConstantsBinding = 1,

    DiffuseTexImageUnit = 0,
  };

  gx::Pipeline createPipeline();
  u32 createFramebuffer();
  u32 createRenderPass();
  u32 createConstantBuffer(u32 sz);

  u32 constantBlockSizeAlign(u32 sz);

  ShaderConstants generateSceneConstants();
  // Returns an offset into the ObjectConstants UniformBuffer
  //   where the element under that offset describes 'ro'
  u32 writeConstants(const RenderObject& ro);

  // Writes ObjectsConstants and commands (into 'cmd') needed to
  //   render 'ro'
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

  // Stores the currently used render tagrets
  std::vector<const RenderTarget *> m_rts;

  u32 m_renderpass_id;

  u32 m_scene_ubo_id;
  u32 m_object_ubo_id;

  // Used by writeConstants() to find the new RenderObject's constants
  //   offset in the current block
  size_t m_num_objects_per_block;

  // Stores the beginning of the ObjectConstants UniformBuffer mapping
  ObjectConstants *m_objects = nullptr;
  // Stores the current write offset into the ObjectConstants UniformBuffer
  //   mapping
  StridePtr<ObjectConstants> m_objects_rover;
  // Stores the end of the ObjectConstants UniformBuffer mapping
  ObjectConstants *m_objects_end = nullptr;
};

}