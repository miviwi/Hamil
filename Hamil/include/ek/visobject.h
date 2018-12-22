#pragma once

#include <ek/euklid.h>

#include <util/smallvector.h>
#include <math/geometry.h>
#include <math/util.h>

#include <utility>
#include <array>
#include <vector>
#include <memory>

//#define NO_OCCLUSION_SSE

namespace ek {

// Structure of arrays
struct VisMesh4Tris {
  // v[0] == x0 x1 x2 x3
  // v[1] == y0 y1 y2 y3
  // v[2] == z0 z1 z2 z3
  // v[3] == w0 w1 w2 w3
  __m128 v[4];
};

// See note for XformedPtr below
void free_xformed(vec4 *v);

// Stores pointers to vertex and index data for a mesh
//   along with an array of transformed vertices
struct VisibilityMesh {
  using Triangle = std::array<vec4, 3>;

  mat4 model = mat4::identity();

  AABB aabb;

  u32 num_verts = 0;
  u32 num_inds = 0;

  StridePtr<const vec3> verts = { nullptr, 0 };

  // This 'Deleter' function is used to allow allocation of the backing
  //   array for the std::unique_ptr via malloc() which avoids the overhead
  //   of default constructing the vec4[] (which malloc() doesn't do,
  //   unlike operator new[])
  //  - Insignificant, but from profiling it seens to help
  using XformedPtr = std::unique_ptr<vec4[], decltype(&free_xformed)>;
  XformedPtr xformed = { nullptr, nullptr };

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

    auto xformed_ptr = (vec4 *)malloc(self.num_verts * sizeof(vec4));
    self.xformed = XformedPtr(xformed_ptr, free_xformed);

    self.inds = inds.data();

    return std::move(self);
  }

  // Returns the transformed vertices for triangle formed
  //   from indices at offset idx*3 in 'inds'
  Triangle gatherTri(uint idx) const;
  // Returns the transformed vertices for 'num_lanes' (which must be <= 4)
  //   consecutive triangles formed from indices at offset idx*3 in 'inds'
  void gatherTri4(VisMesh4Tris tris[3], uint idx, uint num_lanes) const;

  // Returns the number of triangles in this mesh
  //   - num_inds / 3
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
