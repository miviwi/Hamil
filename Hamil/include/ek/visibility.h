#pragma once

#include <ek/euklid.h>
#include <ek/visobject.h>
#include <ek/occlusion.h>

#include <math/geometry.h>
#include <math/frustum.h>

#include <vector>
#include <optional>
#include <memory>

namespace ek {

class MemoryPool;

class ViewVisibility {
public:
  using ObjectsVector = std::vector<VisibilityObject *>;

  ViewVisibility(MemoryPool& mempool);

  // Sets the projection * view matrix which will
  //   be used to transform VisibilityMeshes
  ViewVisibility& viewProjection(const mat4& vp);
  // Sets the near plane distance
  ViewVisibility& nearDistance(float n);

  // Adds an occluder to an internal array
  //  - The added object must be freed by the caller
  ViewVisibility& addObjectRef(VisibilityObject *object);

  // Same as addObjectRef() except 'object' will
  //   be disposed of by the ViewVisibility
  ViewVisibility& addObject(VisibilityObject *object);

  // Calls VisibilityObject::transformMeshes() on all
  //   VisibilityObjects added by addObject()
  // - viewProjection() must've been called before this method!
  ViewVisibility& transformOccluders();

  // - transformOccluders() must be called before this method!
  ViewVisibility& binTriangles();
  // binTriangles() must be called before this method
  //   or nothing will be rendered into occlusionBuf()!
  ViewVisibility& rasterizeOcclusionBuf();

  const OcclusionBuffer& occlusionBuf() const;

  // Fills in VisibilityMesh::visible and VisibilityMesh::vis_flags
  //   for each mesh contained in the VisibilityObject
  //   according to the contents of the occlusionBuf()
  ViewVisibility& occlusionQuery(VisibilityObject *vis);

private:
  using OwnedObjectsVector = std::vector<VisibilityObject::Ptr>;

  mat4 m_viewprojection;
  mat4 m_viewprojectionviewport; // OcclusionBuffer::ViewportMatrix * m_viewprojection
  float m_near;  // Distance to the near plane

  frustum3 m_frustum = { mat4::identity() };

  ObjectsVector m_objects;
  OcclusionBuffer m_occlusion_buf;

  // Using a std::optional here to avoid unnecessary
  //   heap allocations when addObject() is never
  //   called on this ViewVisibility
  std::optional<OwnedObjectsVector> m_owned_objects;
};

}