#include <ek/visobject.h>
#include <ek/occlusion.h>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <emmintrin.h>

#include <cassert>
#include <cstdlib>

namespace ek {

void free_xformed(vec4 *v)
{
  free(v);
}

uint VisibilityObject::flags() const
{
  return m_flags;
}

VisibilityObject& VisibilityObject::flags(uint f)
{
  m_flags = f;

  return *this;
}

const AABB& VisibilityObject::aabb() const
{
  return m_aabb;
}

VisibilityMesh& VisibilityObject::mesh(uint idx)
{
  return m_meshes.at(idx);
}

const VisibilityMesh& VisibilityObject::mesh(uint idx) const
{
  return m_meshes.at(idx);
}

uint VisibilityObject::numMeshes() const
{
  return (uint)m_meshes.size();
}

VisibilityObject& VisibilityObject::addMesh(VisibilityMesh&& mesh)
{
  // Update the cummulative AABB
  m_aabb = {
    vec3::min(m_aabb.min, mesh.aabb.min),
    vec3::max(m_aabb.max, mesh.aabb.max)
  };

  m_meshes.emplace_back(std::move(mesh));

  return *this;
}

VisibilityObject& VisibilityObject::transformMeshes(const mat4& viewprojectionviewport,
  const frustum3& frustum)
{
  // Transform all the meshes into screen space
  for(auto& mesh : m_meshes) {
    auto mvp = viewprojectionviewport * mesh.model;
    transformOne(mesh, mvp);
  }

  return *this;
}

VisibilityObject& VisibilityObject::transformAABBs()
{
  for(auto& mesh : m_meshes) {
    const auto& model = mesh.model;
    const auto& aabb  = mesh.aabb;

    vec4 min = model * vec4(aabb.min, 1.0f);
    vec4 max = model * vec4(aabb.max, 1.0f);

    mesh.transformed_aabb.min = min.xyz();
    mesh.transformed_aabb.max = max.xyz();
  }

  return *this;
}

VisibilityObject& VisibilityObject::frustumCullMeshes(const frustum3& frustum)
{
  for(auto& mesh : m_meshes) {
    if(!frustum.aabbInside(mesh.transformed_aabb)) {
      mesh.visible = VisibilityMesh::Invisible;
      mesh.vis_flags = VisibilityMesh::FrustumOut;
    } else {
      mesh.visible = VisibilityMesh::PotentiallyVisible;
      mesh.vis_flags = 0;
    }
  }

  return *this;
}

static constexpr uint NumAABBVerts = 8;

VisibilityObject& VisibilityObject::occlusionCullMeshes(
  OcclusionBuffer& occlusion, const mat4& viewprojectionviewport)
{
#if defined(NO_OCCLUSION_SSE)
  assert(0 && "OcclusionBuffer::occlusionQuery() scalar version unimplemented");
#endif

  for(auto& mesh : m_meshes) {
    // Check if the mesh had already been culled in an earlier stage
    if(mesh.visible == VisibilityMesh::Invisible) continue;

    // Result of viewprojectionviewport * aabb_verts
    //   - Used to avoid recalculating the transform
    //     in case a fullTest() is needed
    __m128 xformed[NumAABBVerts];

    auto early_test = occlusion.earlyTest(mesh, viewprojectionviewport, xformed);
    if(early_test == VisibilityMesh::Invisible) {
      // The earlyTest() succesufully determined the mesh
      //   is occluded, so no fullTest() is needed
      mesh.visible = VisibilityMesh::Invisible;
    } else if(early_test == VisibilityMesh::Unknown) {
      // The earlyTest() was inconclusive - need to run the fullTest()
      auto full_test = occlusion.fullTest(mesh, viewprojectionviewport, xformed);
      if(!full_test) {  // Mesh is occluded
        mesh.visible = VisibilityMesh::Invisible;
      } else {          // Mesh is (at least partially) visible
        mesh.visible = VisibilityMesh::Visible;
      }

      mesh.vis_flags = VisibilityMesh::LateOut;
      continue;
    } else {
      // The earlyTest() determined the mesh
      //   must be (at least partially) visible,
      //   so there is no need to run the fullTest()
      mesh.visible = VisibilityMesh::Visible;
    }

    mesh.vis_flags = VisibilityMesh::EarlyOut;
  }

  return *this;
}

void VisibilityObject::transformOne(VisibilityMesh& mesh, const mat4& mvp)
{
  auto in  = mesh.verts;
#if defined(NO_OCCLUSION_SSE)
  auto out = mesh.xformed.get();
#else
  auto out = (float *)mesh.xformed.get();

  __m128 row0 = _mm_load_ps(mvp.d +  0);
  __m128 row1 = _mm_load_ps(mvp.d +  4);
  __m128 row2 = _mm_load_ps(mvp.d +  8);
  __m128 row3 = _mm_load_ps(mvp.d + 12);

  _MM_TRANSPOSE4_PS(row0, row1, row2, row3)
#endif

  for(size_t i = 0; i < mesh.num_verts; i++) {
#if defined(NO_OCCLUSION_SSE)
    auto v = vec4(*in++, 1.0f);
    v = mvp * v;

    // Check if the vertex isn't clipped by the near plane
    if(v.z <= v.w) {
      *out++ = v.perspectiveDivide();
    } else {
      *out++ = vec4::zero();  // If it is set all of it's coords to 0
    }
#else
    auto v = *in++;

    __m128 xformed = row3;    // v.w == 1.0f   =>   row3*v.w == row3
    xformed = _mm_add_ps(xformed, _mm_mul_ps(row0, _mm_load_ps1(&v.x)));
    xformed = _mm_add_ps(xformed, _mm_mul_ps(row1, _mm_load_ps1(&v.y)));
    xformed = _mm_add_ps(xformed, _mm_mul_ps(row2, _mm_load_ps1(&v.z)));

#if !defined(NO_AVX)
#  define splat_ps(a, i) _mm_permute_ps(a, _MM_SHUFFLE(i, i, i, i))
#else
#  define splat_ps(a, i) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(i, i, i, i)))
#endif
    __m128 Z = splat_ps(xformed, 2);
    __m128 W = splat_ps(xformed, 3);
    __m128 projected = _mm_mul_ps(xformed, _mm_rcp_ps(W));

    __m128 no_near_clip = _mm_cmple_ps(Z, W);
    projected = _mm_and_ps(projected, no_near_clip);

    _mm_store_ps(out, projected);
    out += 4;  // vec4
#endif
  }
}

VisibilityMesh& VisibilityMesh::initInternal()
{
  auto xformed_ptr = (vec4 *)malloc(num_verts * sizeof(vec4));
  xformed = XformedPtr(xformed_ptr, free_xformed);

  return *this;
}

VisibilityMesh::Triangle VisibilityMesh::gatherTri(uint idx) const
{
  auto off = idx*3;
  u16 vidx[3] = { inds[off + 0], inds[off + 1], inds[off + 2] };

  // Fetch the indices backwards to reverse the triangle's winding
  //  - Needed because the rasterizer assumes CW while the vertices
  //    are CCW (OpenGL default)
  return { xformed[vidx[2]], xformed[vidx[1]], xformed[vidx[0]] };
}

void VisibilityMesh::gatherTri4(VisMesh4Tris tris[3], uint idx, uint num_lanes) const
{
#if defined(NO_OCCLUSION_SSE)
  assert(0 && "Called gatherTri4() with NO_OCCLUSION_SSE defined!");
#endif

  const u16 *inds0 = inds + idx*3;
  const u16 *inds1 = inds0 + (num_lanes > 1 ? 3 : 0);
  const u16 *inds2 = inds0 + (num_lanes > 2 ? 6 : 0);
  const u16 *inds3 = inds0 + (num_lanes > 3 ? 9 : 0);

  vec4 *in = xformed.get();

  for(uint i = 0; i < 3; i++) {
    uint idx = 2 - i;    // Reverse the winding (see note above)

    __m128 v0 = _mm_load_ps((const float *)(in + inds0[idx]));
    __m128 v1 = _mm_load_ps((const float *)(in + inds1[idx]));
    __m128 v2 = _mm_load_ps((const float *)(in + inds2[idx]));
    __m128 v3 = _mm_load_ps((const float *)(in + inds3[idx]));
    _MM_TRANSPOSE4_PS(v0, v1, v2, v3);    // Convert to SoA

    tris[i].v[0] = v0;
    tris[i].v[1] = v1;
    tris[i].v[2] = v2;
    tris[i].v[3] = v3;
  }
}

uint VisibilityMesh::numTriangles() const
{
  return num_inds / 3;  // Each triangle has 3 vertices
}

}