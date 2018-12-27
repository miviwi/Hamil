#include <ek/visibility.h>

#include <math/frustum.h>

namespace ek {

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

ViewVisibility& ViewVisibility::addObject(VisibilityObject *object)
{
  m_objects.emplace_back(object);

  return *this;
}

ViewVisibility& ek::ViewVisibility::transformOccluders()
{
  frustum3 frustum(m_viewprojection);
  for(auto& o : m_objects) {
    bool is_occluder = o->flags() & VisibilityObject::Occluder;
    if(!is_occluder) continue;

    o->transformMeshes(m_viewprojectionviewport, frustum);
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
  vis->occlusionCullMeshes(m_occlusion_buf, m_viewprojectionviewport);

  return *this;
}

}