#include <ek/visobject.h>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <emmintrin.h>

#include <cstdlib>

namespace ek {

void free_xformed(vec4 *v)
{
  free(v);
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

VisibilityObject& VisibilityObject::transformMeshes(const mat4& viewprojectionviewport)
{
  // Transform all the meshes into screen space
  for(auto& mesh : m_meshes) {
    auto mvp = viewprojectionviewport * mesh.model;
    transformOne(mesh, mvp);

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
    __m128 Z = _mm_permute_ps(xformed, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 W = _mm_permute_ps(xformed, _MM_SHUFFLE(3, 3, 3, 3));
#else
// The macros here don't need an #undef
//   because they don't escape out of
//   their respective translation units
//   anyways
#define ps_epi32(r) _mm_castps_si128(r)
#define epi32_ps(r) _mm_castsi128_ps(r)

    __m128 Z = epi32_ps(_mm_shuffle_epi32(ps_epi32(xformed), _MM_SHUFFLE(2, 2, 2, 2)));
    __m128 W = epi32_ps(_mm_shuffle_epi32(ps_epi32(xformed), _MM_SHUFFLE(3, 3, 3, 3)));
#endif
    __m128 projected = _mm_mul_ps(xformed, _mm_rcp_ps(W));

    __m128 no_near_clip = _mm_cmple_ps(Z, W);
    projected = _mm_and_ps(projected, no_near_clip);

    _mm_store_ps(out, projected);
    out += 4;  // vec4
#endif
  }
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