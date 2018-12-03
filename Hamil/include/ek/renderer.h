#pragma once

#include <ek/euklid.h>
#include <ek/rendertarget.h>
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

class Renderer {
public:
  using ObjectVector = std::vector<RenderObject>;
  using ExtractObjectsJob = std::unique_ptr<
    sched::Job<ObjectVector, hm::Entity, RenderView *>
  >;

  enum {
    // Bump this when things go wrong :)
    InitialRenderTargets = 16,
  };

  Renderer();

  ExtractObjectsJob extractForView(hm::Entity scene, RenderView& view);

  const RenderTarget& queryRenderTarget(const RenderTargetConfig& config, gx::ResourcePool& pool);
  // Remeber to call this after a RenderTarget is no longer in use
  //   so they're not infinitely created
  void releaseRenderTarget(const RenderTarget& rt);

  // Returns a gx::Program which can be used to render 'ro'
  u32 queryProgram(const RenderObject& ro, gx::ResourcePool& pool);

private:
  ObjectVector doExtractForView(hm::Entity scene, RenderView& view);

  // Extracts an Entity and all it's children and appends
  //   them to 'objects'
  void extractOne(ObjectVector& objects, const frustum3& frustum,
    hm::Entity e, const mat4& parent);

  win32::ReaderWriterLock m_rts_lock;
  std::vector<RenderTarget> m_rts;

  win32::ReaderWriterLock m_programs_lock;
  std::vector<u32> m_programs;
};

}