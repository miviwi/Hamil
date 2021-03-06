#pragma once

#include <ek/euklid.h>
#include <ek/rendertarget.h>
#include <ek/constbuffer.h>
#include <ek/mempool.h>
#include <ek/renderobject.h>
#include <ek/renderview.h>

#include <sched/job.h>
#include <os/rwlock.h>
#include <hm/entity.h>
#include <hm/componentref.h>

#include <vector>
#include <map>
#include <set>
#include <memory>

namespace gx {
class ResourcePool;
}

namespace hm {
struct Transform;
struct Light;
}

namespace ek {

// PIMPL class
class RendererData;

struct RenderLUT {
  enum Type : size_t {
    // 1D separable Gaussian kernel => param == radius
    // gx::Texture1D where the first param*2 + 1 texels
    //   are the kernel itself and the last texel is the
    //   normalization factor
    GaussianKernel,

    // gx::Texture2DArray, where:
    //   - Layer 0 containts the inverse transform
    //     matrix coefficients
    //        vec4(m11, m13, m31, m33)
    //   - Layer 1 contains a
    //        vec4(norm_factor, fresnel, 0, clipped_form_factor)
    LTCCoeffs,

    // param == number of kernel directions (defaults to 8)
    // gx::Texture2D, which stores:
    //    vec3(cos(rotation), sin(rotation), start_offset)
    HBAONoise,

    NumTypes
  };

  RenderLUT();

  Type type;
  int param;  // Optional parameter (eg. blur radius)

  u32 tex_id; // Id of the texture which stores the LUT

  // After filling in 'type' (and 'param if needed)
  //   calling this method will populate 'tex_id'
  //   with a gx::ResourcePool::Id of a texture
  //   which contains the requested LUT
  void generate(gx::ResourcePool& pool);

private:
  void generateGaussian(gx::ResourcePool& pool);
  void generateLTC(gx::ResourcePool& pool);
  void generateHBAONoise(gx::ResourcePool& pool);
};

enum SamplerClass {
  MSMTrilinearSampler,
  PCFShadowMapSampler,
  HBAONoiseSampler,  // gx::Sampler::repeat2d()

  LUT1DNearestSampler,
  LUT2DLinearSampler,
};

class Renderer {
public:
  using ObjectVector = std::vector<RenderObject>;
  using ExtractObjectsJob = std::unique_ptr<
    sched::Job<ObjectVector, hm::Entity, RenderView *>
  >;

  enum {
    // Bump these when things go wrong :)
    //   - RenderViews store pointers to objects allocated
    //     inside std::vector's so to avoid invalidating them
    //     when out of capacity, Renderer's constructor calls
    //     vector::reserve() with the parameters below
    // TODO: maybe do this better somehow?

    InitialRenderTargets   = 16,
    InitialConstantBuffers = 256,
    InitialMemoryPools     = 32,
  };

  Renderer();
  ~Renderer();

  // ResourcePool used by all RenderViews
  //   - Put all VertexArrays, Textures, etc. used for rendering
  //     here
  gx::ResourcePool& pool();

  // Pre-compile all GPU programs
  //   - Should be called before doing any rendering to
  //     avoid stuttering when encountering a new shader
  Renderer& cachePrograms();

  // Generates the data stream for RenderView::render(),
  //   and populates it with RenderLights
  ExtractObjectsJob extractForView(hm::Entity scene, RenderView& view);

  // Returns a RenderTarget compatible with 'config', recycling one
  //   used previously if possible
  //  - Remeber to call releaseRenderTarget()!
  //  - The gx::Fence referenced by 'fence_id' will be used to
  //    guard the returned RenderTarget so it's not reused
  //    before Fence::signaled() == true
  const RenderTarget& queryRenderTarget(const RenderTargetConfig& config, u32 fence_id);
  // Remeber to call this after a RenderTarget is no longer in use
  //   so they can be recycled
  void releaseRenderTarget(const RenderTarget& rt);

  // Returns a gx::Program which can be used to render 'ro'
  //   - The programs can be used concurrently so there
  //     only ever exists one instance of a given program,
  //     hence there is no need to release them
  u32 queryProgram(const RenderView& view, const RenderObject& ro);

  // Returns a ConstantBuffer with size() >= sz, recycling one
  //   used previously if possible
  //  - Remeber to call releaseConstantBuffer()!
  //  - The gx::Fence referenced by 'fence_id' will be used to
  //    guard the returned ConstantBuffer so it's not reused
  //    before Fence::signaled() == true
  const ConstantBuffer& queryConstantBuffer(size_t sz, u32 fence_id, const std::string& label = "");
  // Remeber to call this after a ConstantBuffer is no longer in use
  //   so they can be recycled
  void releaseConstantBuffer(const ConstantBuffer& buf);

  // Returns RenderLUT.tex_id of a RenderLUT with the
  //  requested parameters
  //   - The LUTs are read-only so they can be used
  //     concurrently and there is no need to release them
  //     (same as queryProgram())
  u32 queryLUT(RenderLUT::Type type, int param = -1);

  // Samplers can be use concurrently so there is no
  //   need to release them
  u32 querySampler(SamplerClass sampler);
   
  // Returns a ResourcePool::Id of a new Fence
  //   - Remeber to call doneFence() once Fence::sync()
  //     has been called on it, so it can be
  //     disposed of!
  u32 queryFence(const std::string& label);
  // Call to mark the Fence referenced by 'id'
  //   for deletion (it will be disposed of
  //   once all SharedObjects guarded by it
  //   have been re-acquired)
  //  - Passing in 'id' == gx::ResourcePool::Invalid
  //    is a no-op
  void doneFence(u32 id);

  MemoryPool& queryMempool(size_t sz, u32 fence_id);
  void releaseMempool(MemoryPool& mempool);

private:
  // Fill 'm_luts' with commonly used RenderLUTs
  //   - m_data->pool must be initialized before calling
  //     this method!
  void precacheLUTs();

  // Fill 'm_luts' with commonly used gx::Samplers
  //   - m_data->pool must be initialized before calling
  //     this method!
  void precacheSamplers();

  // ExtractObjectsJob entry point
  ObjectVector doExtractForView(hm::Entity scene, RenderView& view);

  // Extracts an Entity and all it's children and appends
  //   them to 'objects'
  void extractOne(RenderView& view,
    ObjectVector& objects, const frustum3& frustum,
    hm::Entity e, const mat4& parent);

  // Returns 'true' when light was culled
  bool cullLight(RenderView& view,
    const vec3& pos, const hm::Light& light,
    const frustum3& frustum);

  // Stores the gx::ResourcePool of the Renderer
  RendererData *m_data;

  //   RenderTargets
  os::ReaderWriterLock::Ptr m_rts_lock;
  std::vector<RenderTarget> m_rts;

  //   Programs
  os::ReaderWriterLock::Ptr m_programs_lock;
  std::vector<u32> m_programs;

  //   ConstantBuffers
  os::ReaderWriterLock::Ptr m_const_buffers_lock;
  std::vector<ConstantBuffer> m_const_buffers;

  //   RenderLUTs
  os::ReaderWriterLock::Ptr m_luts_lock;
  std::vector<RenderLUT> m_luts;

  //   Samplers
  os::ReaderWriterLock::Ptr m_samplers_lock;
  std::map<SamplerClass, u32> m_samplers;

  //   Fences
  os::ReaderWriterLock::Ptr m_fences_lock;
  std::set<u32> m_fences;  // std::set for fast removal

  //  MemoryPools
  os::ReaderWriterLock::Ptr m_mempools_lock;
  std::vector<MemoryPool> m_mempools;
};

}
