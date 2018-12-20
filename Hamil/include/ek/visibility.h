#pragma once

#include <ek/euklid.h>
#include <ek/visobject.h>
#include <ek/occlusion.h>

#include <math/geometry.h>

#include <vector>
#include <memory>

namespace ek {

class ViewVisibility {
public:
  // Sets the projection * view matrix which will
  //   be used to transform VisibilityMeshes
  ViewVisibility& viewProjection(const mat4& vp);
  // Sets the near plane distance
  ViewVisibility& nearDistance(float n);

  // Adds an occluder to an internal array
  //   - 'object' should be allocated with new
  //     and doesn't need to have delete called
  //     on it (the ViewVisibility object takes
  //     care of it)
  ViewVisibility& addObject(VisibilityObject *object);

  // Calls VisibilityObject::transformMeshes() on all
  //   VisibilityObjects added by addObject()
  ViewVisibility& transformObjects();

  ViewVisibility& binTriangles();
  ViewVisibility& rasterizeOcclusionBuf();

  const OcclusionBuffer& occlusionBuf() const;

private:
  mat4 m_viewprojection;
  mat4 m_viewprojectionviewport;
  float m_near;  // Distance to the near plane

  std::vector<VisibilityObject::Ptr> m_objects;
  OcclusionBuffer m_occlusion_buf;
};

}