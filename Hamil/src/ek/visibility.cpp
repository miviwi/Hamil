#include <ek/visibility.h>

#include <math/frustum.h>

namespace ek {

ViewVisibility& ViewVisibility::viewProjection(const mat4& vp)
{
  m_viewprojection = vp;
  m_viewprojectionviewport = OcclusionBuffer::ViewportMatrix * vp;

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

ViewVisibility& ek::ViewVisibility::transformObjects()
{
  frustum3 frustum(m_viewprojection);

  for(auto& o : m_objects) {
    // Frustum cull the VisibilityObjects
    auto aabb = o->aabb();
    if(!frustum.aabbInside(aabb)) continue;

    o->transformMeshes(m_viewprojectionviewport);
  }

  return *this;
}

ViewVisibility& ViewVisibility::binTriangles()
{
  m_occlusion_buf.binTriangles(m_objects);

  return *this;
}

ViewVisibility & ViewVisibility::rasterizeOcclusionBuf()
{
  m_occlusion_buf.rasterizeBinnedTriangles(m_objects);

  return *this;
}

const OcclusionBuffer& ViewVisibility::occlusionBuf() const
{
  return m_occlusion_buf;
}

}