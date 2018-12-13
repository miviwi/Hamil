#pragma once

#include <ek/euklid.h>

#include <util/ref.h>
#include <math/geometry.h>
#include <math/frustum.h>

#include <array>
#include <vector>
#include <set>

namespace gx {
class Pipeline;
class RenderPass;
class CommandBuffer;
class ResourcePool;
class MemoryPool;
class UniformBuffer;
}

namespace ek {

class Renderer;
class RenderTarget;
class RenderObject;
class RenderMesh;
class RenderLight;
class ConstantBuffer;

// Stores a MemoryPool Handle and a size
struct ShaderConstants;

struct ObjectConstants;
struct LightConstants;

// PIMPL class
class RenderViewData;

// Must keep the RenderView around until render() finishes
class RenderView : public Ref {
public:
  enum ViewType {
    Invalid,

    // Fully lit and postprocessed view
    CameraView,

    // Reflective shadow map
    LightView,

    // Shadow map
    ShadowView,

    NumViewTypes
  };

  enum RenderType {
    // Used for depth pre-pass
    DepthOnly,

    // Forward rendeing fallback pipeline
    Forward,

    // Deferred pipeline
    Deferred,

    NumRenderTypes,
  };

  static constexpr size_t MempoolInitialAlloc = 4096;

  RenderView(ViewType type);
  ~RenderView();

  // RenderType = DepthOnly
  //   - required for rendering a ShadowView
  RenderView& depthOnlyRender();
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

  vec3 eyePosition() const;

  // Used by Renderer::extractForView()
  frustum3 constructFrustum();

  // 'objects' will be altered by this method
  gx::CommandBuffer render(Renderer& renderer,
    std::vector<RenderObject>& objects);

  // Returns a reference to the RenderTarget which holds
  //   the rendered view
  const RenderTarget& presentRenderTarget() const;

  RenderView& addInput(const RenderView *input);

private:
  friend Renderer;

  enum : u32 {
    SceneConstantsBinding  = 0,
    ObjectConstantsBinding = 1,

    NumConstantBufferBindings = 2,

    DiffuseTexImageUnit    = 0,
    ShadowMapTexImageUnit  = 1,
    BlurKernelTexImageUnit = 2,
    LTCCoeffsTexImageUnit = 3,
  };

  constexpr static int GaussianBlurRadius = 1;  // See math/util.h

  std::string labelPrefix() const;

  // m_renderer->pool()
  gx::ResourcePool& pool();

  gx::Pipeline createPipeline();

  u32 createFramebuffer();

  u32 createRenderPass();
  u32 createForwardRenderPass();
  u32 createShadowRenderPass();

  u32 constantBufferId(u32 which);
  gx::UniformBuffer& constantBuffer(u32 which);

  // Aligns 'sz' to gx::info().minUniformBindAlignment()
  //   (i.e. the minimum required alignment of a uniform
  //    block's bind range)
  u32 constantBlockSizeAlign(u32 sz);

  // pool().get<gx::RenderPass>(m_renderpass_id)
  gx::RenderPass& getRenderpass();

  // Initializes:
  //   - m_num_objects_per_block, m_constant_block_sz
  //   - m_objects, m_objects_rover, m_objects_end
  //   - UniformBuffer bindings on the current RenderPass,
  //     which means m_renderpass_id MUST be initialized
  //     before calling this function
  void initConstantBuffers(size_t num_ros);
  // Rebinds the ObjectCOnstants UniformBuffer every
  //   m_num_objects_per_block RenderObjects
  //  - Must be called AFTER the appropriate RenderFn
  void advanceConstantBlockBinding(gx::CommandBuffer& cmd);

  // Fills MemoryPool block 'h' with 'sz' bytes of
  //   scene constant data
  ShaderConstants generateSceneConstants();
  // Returns an offset into the ObjectConstants UniformBuffer
  //   where the element under that offset describes 'ro'
  u32 writeConstants(const RenderObject& ro);

  // Calls Renderer::queryLUT()
  void initLuts();

  // Returns the number of processed lights (i.e. the starting
  //  index of the RenderMeshes in 'objects')
  // - 'objects' MUST have been sorted before calling
  //   this method!
  // - generateSceneConstants() MUST have been called
  //   before this method!
  size_t processLights(const std::vector<RenderObject>& objects);

  // Returns LightConstants for a RenderLight with type Light::Sphere
  LightConstants generateSphereLightConstants(const RenderObject& ro);
  // Returns LightConstants for a RenderLight with type Light::Line
  LightConstants generateLineLightConstants(const RenderObject& ro);

  using RenderFn = void (RenderView::*)(const RenderObject&, gx::CommandBuffer&);
  static const RenderFn RenderFns[NumViewTypes][NumRenderTypes];

  // m_type == CameraView, m_render == Forward
  void forwardCameraRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd);

  // m_type == ShadowView, m_render == DepthOnly
  void shadowRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd);

  // Emits the draw command for ro.mesh()
  void emitDraw(const RenderMesh& ro, gx::CommandBuffer& cmd);

  ViewType m_type;
  RenderType m_render;

  // gx::info().minUniformBindAlignment()
  u32 m_ubo_alignment;
  // gx::info().maxUniformBlockSize()
  u32 m_constant_block_sz;

  // x, y, width, height
  ivec4 m_viewport;

  // 0 == no MSAA
  uint m_samples;

  mat4 m_view;     // View matrix
  mat4 m_projection;  // Projection matrix (ViewType dependent)

  // Stores mapped BufferViews and other working data
  RenderViewData *m_data;

  // Assigned in render()
  Renderer *m_renderer;
  // Allocated in render()
  gx::MemoryPool *m_mempool;

  // Provided by the user (see the addInput() method)
  std::vector<const RenderView *> m_inputs;

  // Stores the currently used render tagrets
  std::vector<const RenderTarget *> m_rts;

  // Stores Ids of gx::Programs which already had their
  //   uniformBlockBindings, samplers, etc. set
  std::set<u32> m_init_programs;

  // Indexed via <...>ConstantsBinding enum values
  std::vector<const ConstantBuffer *> m_const_bufs;

  u32 m_renderpass_id;

  // Used by writeConstants() to find the new RenderObject's constants
  //   offset in the current block
  size_t m_num_objects_per_block = ~0ull;

  // Stores the beginning of the ObjectConstants UniformBuffer mapping
  ObjectConstants *m_objects = nullptr;
  // Stores the current write offset into the ObjectConstants UniformBuffer
  //   mapping
  StridePtr<ObjectConstants> m_objects_rover;
  // Stores the end of the ObjectConstants UniformBuffer mapping
  ObjectConstants *m_objects_end = nullptr;
};

}