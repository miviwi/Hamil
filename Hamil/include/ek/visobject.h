#pragma once

#include <ek/euklid.h>

#include <util/smallvector.h>
#include <math/geometry.h>
#include <math/util.h>

#include <utility>
#include <array>
#include <vector>
#include <memory>

namespace ek {

union VisMesh4Tris {
  struct { __m128 X, Y, Z, W; };
  __m128 v[4];
};

// Stores pointers to vertex and index data for a mesh
//   along with an array of transformed vertices
struct VisibilityMesh {
  using Triangle = std::array<vec4, 3>;

  mat4 model = mat4::identity();

  AABB aabb;

  u32 num_verts = 0;
  u32 num_inds = 0;

  StridePtr<const vec3> verts = { nullptr, 0 };
  std::unique_ptr<vec4[]> xformed;

  const u16 *inds = nullptr;

  template <typename VertsVec>
  static VisibilityMesh from_vectors(const mat4& model, const AABB& aabb,
    const VertsVec& verts, const std::vector<u16>& inds)
  {
    VisibilityMesh self;

    self.model = model;
    self.aabb = aabb;

    self.num_verts = (u32)verts.size();
    self.num_inds  = (u32)inds.size();

    self.verts = StridePtr<const vec3>(
      /* cast to silence IntelliSense */ (void *)verts.data(),
      sizeof(VertsVec::value_type)
    );
#if defined(NO_OCCLUSION_SSE)
    self.xformed = std::make_unique<vec4[]>(self.num_verts);
#else
    self.xformed = std::make_unique<vec4[]>(pow2_align(self.num_verts, 16));
#endif

    self.inds = inds.data();

    return std::move(self);
  }

  // Returns the transformed vertices for triangle formed
  //   from indices at offset idx*3 in 'inds'
  Triangle gatherTri(uint idx) const;

  void gatherTri4(VisMesh4Tris tris[3], uint idx, uint num_lanes) const;

  uint numTriangles() const;
};

class VisibilityObject {
public:
  using Ptr = std::unique_ptr<VisibilityObject>;

  const AABB& aabb() const;

  VisibilityMesh& mesh(uint idx);
  const VisibilityMesh& mesh(uint idx) const;

  // Returns the max_index+1 which can be passed to mesh()
  uint numMeshes() const;

  VisibilityObject& addMesh(VisibilityMesh&& mesh);

  // Fills VisibilityMesh::xformed arrays with
  //   screen space vertex coords
  VisibilityObject& transformMeshes(const mat4& viewprojectionviewport);

private:
  void transformOne(VisibilityMesh& mesh, const mat4& mvp);

  AABB m_aabb = { vec3(INFINITY), vec3(-INFINITY) };

  std::vector<VisibilityMesh> m_meshes;
};

}
