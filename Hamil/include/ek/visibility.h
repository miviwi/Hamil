#pragma once

#include <ek/euklid.h>
#include <ek/visobject.h>
#include <ek/occlusion.h>

#include <math/geometry.h>
#include <math/frustum.h>

#include <vector>
#include <memory>

namespace ek {

class ViewVisibility {
public:
  using ObjectsVector = std::vector<VisibilityObject *>;

  // Sets the projection * view matrix which will
  //   be used to transform VisibilityMeshes
  ViewVisibility& viewProjection(const mat4& vp);
  // Sets the near plane distance
  ViewVisibility& nearDistance(float n);

  // Adds an occluder to an internal array
  //  - The added object must be freed by the caller
  ViewVisibility& addObject(VisibilityObject *object);

  // Calls VisibilityObject::transformMeshes() on all
  //   VisibilityObjects added by addObject()
  ViewVisibility& transformOccluders();

  ViewVisibility& binTriangles();
  ViewVisibility& rasterizeOcclusionBuf();

  const OcclusionBuffer& occlusionBuf() const;

  ViewVisibility& occlusionQuery(VisibilityObject *vis);

private:
  mat4 m_viewprojection;
  mat4 m_viewprojectionviewport;
  float m_near;  // Distance to the near plane

  frustum3 m_frustum = { mat4::identity() };

  ObjectsVector m_objects;
  OcclusionBuffer m_occlusion_buf;
};

}