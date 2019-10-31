#include <ek/visibility.h>

#include <util/unit.h>
#include <math/frustum.h>
#include <sched/pool.h>
#include <sched/job.h>

namespace ek {

ViewVisibility::ViewVisibility(MemoryPool& mempool) :
  m_mempool(&mempool),
  m_occlusion_buf(mempool)
{
}

ViewVisibility& ViewVisibility::viewProjection(const mat4& vp)
{
  m_viewprojection = vp;
  m_viewprojectionviewport = OcclusionBuffer::ViewportMatrix * vp;

  m_frustum = frustum3(vp);

  return *this;
}

ViewVisibility& ViewVisibility::nearDistance(float n)
{
  m_near = n;

  return *this;
}

ViewVisibility& ViewVisibility::addObjectRef(VisibilityObject *object)
{
  m_objects.emplace_back(object);  // Don't add 'object' to 'm_owned_objects'
                                   //   - The caller is responsible for
  return *this;                    //     freeing it
}

ViewVisibility& ViewVisibility::addObject(VisibilityObject *object)
{
  if(m_owned_objects) {
    m_owned_objects->emplace_back(object);
  } else {
    // 'm_owned_objects' hasn't been created yet
    m_owned_objects.emplace();

    // This call will execute the first branch of the if
    return addObject(object);
  }

  m_objects.emplace_back(object);

  return *this;
}

ViewVisibility& ek::ViewVisibility::transformOccluders(sched::WorkerPool& pool)
{
  using TransformJobData = std::pair<sched::WorkerPool::JobId, sched::IJob *>;

  m_next_object.store(0);

  std::array<TransformJobData, NumJobs> jobs;
  for(uint job_idx = 0; job_idx < NumJobs; job_idx++) {
    sched::IJob *job = new sched::Job<Unit>(sched::create_job([this]() -> Unit {
      uint i = 0;
      while((i = m_next_object.fetch_add(1)) < m_objects.size()) {
        auto& o = m_objects[i];

        bool is_occluder = o->flags() & VisibilityObject::Occluder;
        if(!is_occluder) continue;

        o->foreachMesh([this](VisibilityMesh& mesh) {
          mesh.initInternal(*m_mempool)
            .transform(m_viewprojectionviewport, m_frustum);
        });
      }

      return {};
    }));

    auto id = pool.scheduleJob(job);
    jobs[job_idx] = std::make_pair(id, job);
  }

  // Wait for all occluders to be transformed by the workers
  for(const auto& job : jobs) {
    pool.waitJob(job.first);
    delete job.second;
  }

  return *this;
}

ViewVisibility& ViewVisibility::binTriangles()
{
  m_occlusion_buf.binTriangles(m_objects);

  return *this;
}

ViewVisibility& ViewVisibility::rasterizeOcclusionBuf(sched::WorkerPool& pool)
{
  m_occlusion_buf.rasterizeBinnedTriangles(m_objects, pool);

  return *this;
}

const OcclusionBuffer& ViewVisibility::occlusionBuf() const
{
  return m_occlusion_buf;
}

ViewVisibility& ViewVisibility::occlusionQuery(VisibilityObject *vis)
{
  vis->transformAABBs();
  vis->frustumCullMeshes(m_frustum);
#if !defined(NO_OCCLUSION_SSE)
  vis->occlusionCullMeshes(m_occlusion_buf, m_viewprojectionviewport);
#endif

  return *this;
}

}
