#pragma once

#include <ek/euklid.h>
#include <ek/rendertarget.h>
#include <ek/constbuffer.h>
#include <ek/renderobject.h>
#include <ek/renderview.h>

#include <sched/job.h>
#include <win32/rwlock.h>
#include <hm/entity.h>
#include <hm/componentref.h>

#include <vector>
#include <memory>

namespace gx {
class ResourcePool;
}

namespace hm {
struct Transform;
}

namespace ek {

// PIMPL class
class RendererData;

struct RenderLUT {
  enum Type : size_t {
    GaussianKernel,
    LTC_Coeffs,

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
};

class RenderLight {

};

class Renderer {
public:
  using ObjectVector = std::vector<RenderObject>;
  using ExtractObjectsJob = std::unique_ptr<
    sched::Job<ObjectVector, hm::Entity, RenderView *>
  >;

  using LightVector = std::vector<RenderLight>;
  using ExtractLightsJob = std::unique_ptr<
    sched::Job<LightVector, hm::Entity, RenderView *>
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
  };

  Renderer();
  // TODO (!): ~Renderer();

  // ResourcePool used by all RenderViews
  //   - Put all VertexArrays, Textures, etc. used for rendering
  //     here
  gx::ResourcePool& pool();

  // Pre-compile all GPU programs
  //   - Should be called before doing any rendering to
  //     avoid stuttering when encountering a new shader
  Renderer& cachePrograms();

  // Generates the data stream for RenderView::render()
  ExtractObjectsJob extractForView(hm::Entity scene, RenderView& view);

  // Returns a RenderTarget compatible with 'config', recycling one
  //   used previously if possible
  //  - Remeber to call releaseRenderTarget()!
  const RenderTarget& queryRenderTarget(const RenderTargetConfig& config);
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
  // - Remeber to call releaseConstantBuffer()!
  const ConstantBuffer& queryConstantBuffer(size_t sz);
  // Remeber to call this after a ConstantBuffer is no longer in use
  //   so they can be recycled
  void releaseConstantBuffer(const ConstantBuffer& buf);

  // Returns RenderLUT.tex_id of a RenderLUT with the
  //  requested parameters
  //   - The LUTs are read-only so they can be used
  //     concurrently and there is no need to release them
  //     (same as queryProgram())
  u32 queryLUT(RenderLUT::Type type, int param = -1);

private:
  // ExtractObjectsJob entry point
  ObjectVector doExtractForView(hm::Entity scene, RenderView& view);

  // Extracts an Entity and all it's children and appends
  //   them to 'objects'
  void extractOne(ObjectVector& objects, const frustum3& frustum,
    hm::Entity e, const mat4& parent);

  // Stores the gx::ResourcePool of the Renderer
  RendererData *m_data;

  //   RenderTargets
  win32::ReaderWriterLock m_rts_lock;
  std::vector<RenderTarget> m_rts;

  //   Programs
  win32::ReaderWriterLock m_programs_lock;
  std::vector<u32> m_programs;

  //   ConstantBuffers
  win32::ReaderWriterLock m_const_buffers_lock;
  std::vector<ConstantBuffer> m_const_buffers;

  //   RenderLUTs
  win32::ReaderWriterLock m_luts_lock;
  std::vector<RenderLUT> m_luts;
};

}