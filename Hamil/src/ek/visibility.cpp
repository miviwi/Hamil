#include <ek/visibility.h>

#include <math/frustum.h>

namespace ek {

ViewVisibility::ViewVisibility(MemoryPool& mempool) :
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

  return *this;
}

ViewVisibility& ek::ViewVisibility::transformOccluders()
{
  for(auto& o : m_objects) {
    bool is_occluder = o->flags() & VisibilityObject::Occluder;
    if(!is_occluder) continue;

    o->transformMeshes(m_viewprojectionviewport, m_frustum);
  }

  return *this;
}

ViewVisibility& ViewVisibility::binTriangles()
{
  m_occlusion_buf.binTriangles(m_objects);

  return *this;
}

ViewVisibility& ViewVisibility::rasterizeOcclusionBuf()
{
  m_occlusion_buf.rasterizeBinnedTriangles(m_objects);

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