#include <ek/occlusion.h>

#include <utility>

#include <intrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

// All of the following source code is heavily based on
//  https://github.com/GameTechDev/OcclusionCulling

namespace ek {

// Clip triangle to bounding box defined by <min_extent; max_extent>
static std::pair<ivec2, ivec2> tri_bbox(ivec2 fx[3], ivec2 min_extent, ivec2 max_extent)
{
  ivec2 start = ivec2::max(ivec2::min(ivec2::min(fx[0], fx[1]), fx[2]), min_extent);
  ivec2 end   = ivec2::min(ivec2::max(ivec2::max(fx[0], fx[1]), fx[2]), max_extent);

  return std::make_pair(start, end);
}

static int tri_area(ivec2 fx[3])
{
  return (fx[1].x - fx[0].x)*(fx[2].y - fx[0].y) - (fx[0].x - fx[2].x)*(fx[0].y - fx[1].y);
}

// Implements a fallback for _mm_mulllo_epi32() on pre-SSE41 CPUs
//   - Or uses _mm_mullo_epi32() when available
INTRIN_INLINE static __m128i mullo_epi32(const __m128i& a, const __m128i& b)
{
#if !defined(NO_SSE41)
  return _mm_mullo_epi32(a, b);    // Requires SSE4.1
#else      // Fallback for old CPUs
  __m128i tmp1 = _mm_mul_epu32(a, b);     // mul 2, 0
  tmp1 = _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0));

  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));  // mul 3, 1
  tmp2 = _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0));

  return _mm_unpacklo_epi32(tmp1, tmp2);
#endif
}

// Implements a fallback for _mm_blendv_ps() on pre-SSE41 CPUs
//   - Or uses _mm_blendv_ps() when available
INTRIN_INLINE static __m128 blendv_ps(const __m128& a, const __m128& b, const __m128& mask)
{
#if !defined(NO_SSE41)
  return _mm_blendv_ps(a, b, mask);
#else
  __m128 select = _mm_cmpgt_ps(mask, _mm_setzero_ps());

  return _mm_or_ps(_mm_and_ps(a, select), _mm_andnot_ps(select, b));
#endif
}

#if !defined(NO_AVX)
#  define permute_ps(a, imm) _mm_permute_ps(a, imm)
#else
#  define permute_ps(a, imm) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), imm))
#endif

#define splat_ps(a, i) permute_ps(a, _MM_SHUFFLE(i, i, i, i))

INTRIN_INLINE static int testz_si128(const __m128i& a, const __m128i& b)
{
#if !defined(NO_SSE41)
  return _mm_testz_si128(a, b);
#else
  __m128i band   = _mm_and_si128(a, b);
  __m128i result = _mm_cmpeq_epi32(band, _mm_setzero_si128());
  int mask = _mm_movemask_ps(_mm_castsi128_ps(result));

  return mask == OcclusionBuffer::SIMDLaneMask;
#endif
}

// Find and clear the first (lsb) set bit and return it's index
INTRIN_INLINE static int find_and_clear_lsb(uint *mask)
{
  ulong idx;
  _BitScanForward(&idx, *mask);
  *mask &= *mask - 1;

  return idx;
}

// CLip triangle to bounding box defined by <min_extent; max_extent>
INTRIN_INLINE static void tri_bbox(__m128i fxx[3], __m128i fxy[3], ivec2 min_extent, ivec2 max_extent,
  __m128i& startx, __m128i& starty, __m128i& endx, __m128i& endy)
{
  startx = _mm_max_epi32(
    _mm_min_epi32(_mm_min_epi32(fxx[0], fxx[1]), fxx[2]),
    _mm_set1_epi32(min_extent.x)
  );
  endx = _mm_min_epi32(
    _mm_max_epi32(_mm_max_epi32(fxx[0], fxx[1]), fxx[2]),
    _mm_set1_epi32(max_extent.x)
  );

  starty = _mm_max_epi32(
    _mm_min_epi32(_mm_min_epi32(fxy[0], fxy[1]), fxy[2]),
    _mm_set1_epi32(min_extent.y)
  );
  endy = _mm_min_epi32(
    _mm_max_epi32(_mm_max_epi32(fxy[0], fxy[1]), fxy[2]),
    _mm_set1_epi32(max_extent.y)
  );
}

INTRIN_INLINE static __m128i tri_area(__m128i fxx[3], __m128i fxy[3])
{
  __m128i tri_area1 = _mm_sub_epi32(fxx[1], fxx[0]);
  tri_area1 = mullo_epi32(tri_area1, _mm_sub_epi32(fxy[2], fxy[0]));

  __m128i tri_area2 = _mm_sub_epi32(fxx[0], fxx[2]);
  tri_area2 = mullo_epi32(tri_area2, _mm_sub_epi32(fxy[0], fxy[1]));

  __m128i area = _mm_sub_epi32(tri_area1, tri_area2);

  return area;
}

#if !defined(NO_OCCLUSION_SSE)
static const __m128i ColumnOffsets = _mm_setr_epi32(0, 1, 0, 1);
static const __m128i RowOffsets    = _mm_setr_epi32(0, 0, 1, 1);
#endif

void free_binnedtris(BinnedTri *btri)
{
  free(btri);
}

OcclusionBuffer::OcclusionBuffer()
{
  m_fb = std::make_unique<float[]>(Size.area());

#if defined(NO_OCCLUSION_SSE)
  m_obj_id  = std::make_unique<u16[]>(MaxTriangles);
  m_mesh_id = std::make_unique<u16[]>(MaxTriangles);
  m_bin     = std::make_unique<uint[]>(MaxTriangles);
#else
  auto bin_ptr = (BinnedTri *)malloc(MaxTriangles * sizeof(BinnedTri));
  m_bin = BinnedTrisPtr(bin_ptr, free_binnedtris);

  m_fb_coarse = std::make_unique<vec2[]>(CoarseSize.area());
#endif

  m_bin_counts = std::make_unique<u16[]>(NumBins);
  m_drawn_tris = std::make_unique<u16[]>(NumBins);
}

OcclusionBuffer& OcclusionBuffer::binTriangles(const std::vector<VisibilityObject *>& objects)
{
  // Clear the bin triangle counts
  memset(m_bin_counts.get(), 0, NumBins * sizeof(decltype(m_bin_counts)::element_type));

  for(uint o = 0; o < objects.size(); o++) {
    const auto& obj = objects[o];
    uint num_meshes = obj->numMeshes();
    for(uint m = 0; m < num_meshes; m++) {
      auto& mesh = obj->mesh(m);

      binTriangles(mesh, o, m);
    }
  }

  return *this;
}

OcclusionBuffer& OcclusionBuffer::rasterizeBinnedTriangles(const std::vector<VisibilityObject *>& objects)
{
  static constexpr uint NumTiles = (uint)SizeInTiles.area();
  for(uint i = 0; i < NumTiles; i++) {
    rasterizeTile(objects, i);
  }

  return *this;
}

const float *OcclusionBuffer::framebuffer() const
{
  return m_fb.get();
}

std::unique_ptr<float[]> OcclusionBuffer::detiledFramebuffer() const
{
  auto fb = m_fb.get();
  auto ptr = std::make_unique<float[]>(Size.area());
#if defined(NO_OCCLUSION_SSE)
  for(int y = 0; y < Size.y; y++) {
    auto src = fb + y*Size.x;
    auto dst = ptr.get() + (Size.y - y - 1)*Size.x;
    memcpy(dst, src, Size.x * sizeof(float));
  }
#else
  // Perform detiling and flipping:
  //      y:
  // x:   A B C D  ->  C D . .
  //      . . . .      A B . .
  for(int y = 0; y < Size.y; y += 2) {
    auto src = fb + y*Size.x;
    auto dst = ptr.get() + (Size.y - y - 2)*Size.x;
    for(int x = 0; x < Size.x; x += 2) {
#if defined(NO_OCCLUSION_SSE)
      dst[0] = src[2];
      dst[1] = src[3];
      dst[Size.x + 0 /* Next row */] = src[0];
      dst[Size.x + 1 /* Next row */] = src[1];
#else
      __m128 quad = _mm_load_ps(src);

      _mm_storeh_pi((__m64 *)dst, quad);
      _mm_storel_pi((__m64 *)(dst + Size.x), quad);
#endif

      src += 4;
      dst += 2;
    }
  }
#endif

  return std::move(ptr);
}

const vec2 *OcclusionBuffer::coarseFramebuffer() const
{
  return m_fb_coarse.get();
}

static constexpr uint NumAABBVerts = 8;
static constexpr uint NumAABBInds  = 36;
static constexpr uint NumAABBTris  = NumAABBInds/3;

static constexpr std::array<uint, NumAABBInds> BBoxIndices = {
  0, 1, 2, 0, 2, 3, // Front
  0, 4, 1, 4, 5, 1, // Left
  0, 7, 4, 7, 0, 3, // Top
  3, 2, 6, 6, 7, 3, // Right
  4, 6, 5, 6, 4, 7, // Back
  1, 5, 6, 6, 2, 1, // Bottom
};

// 0 == min corner, 1 == max corner
static constexpr std::array<uint, NumAABBVerts> BBoxXIndices = { 1, 0, 0, 1, 1, 1, 0, 0 };
static constexpr std::array<uint, NumAABBVerts> BBoxYIndices = { 1, 1, 1, 1, 0, 0, 0, 0 };
static constexpr std::array<uint, NumAABBVerts> BBoxZIndices = { 1, 1, 0, 0, 0, 1, 1, 0 };

VisibilityMesh::Visibility OcclusionBuffer::earlyTest(VisibilityMesh& mesh, const mat4& mvp,
  void *xformed_out)
{
  __m128 mmin = _mm_load_ps(mesh.transformed_aabb.min);
  __m128 mmax = _mm_load_ps(mesh.transformed_aabb.max);

  auto xformed = (__m128 *)xformed_out;

  __m128 x_row[2], y_row[2], z_row[2];

  __m128 row0 = _mm_load_ps(mvp.d +  0);
  __m128 row1 = _mm_load_ps(mvp.d +  4);
  __m128 row2 = _mm_load_ps(mvp.d +  8);
  __m128 row3 = _mm_load_ps(mvp.d + 12);
  _MM_TRANSPOSE4_PS(row0, row1, row2, row3);

  x_row[0] = _mm_mul_ps(splat_ps(mmin, 0), row0); x_row[1] = _mm_mul_ps(splat_ps(mmax, 0), row0);
  y_row[0] = _mm_mul_ps(splat_ps(mmin, 1), row1); y_row[1] = _mm_mul_ps(splat_ps(mmax, 1), row1);
  z_row[0] = _mm_mul_ps(splat_ps(mmin, 2), row2); z_row[1] = _mm_mul_ps(splat_ps(mmax, 2), row2);

  __m128 z_all_in = _mm_castsi128_ps(_mm_set1_epi32(~0));
  __m128 screen_min = _mm_set1_ps(INFINITY);
  __m128 screen_max = _mm_set1_ps(-INFINITY);

  for(uint i = 0; i < NumAABBVerts; i++) {
    __m128 v = row3;    // v.w == 1.0f   =>   row3*v.w == row3
    v = _mm_add_ps(v, x_row[BBoxXIndices[i]]);
    v = _mm_add_ps(v, y_row[BBoxYIndices[i]]);
    v = _mm_add_ps(v, z_row[BBoxZIndices[i]]);

    __m128 Z = splat_ps(v, 2);
    __m128 W = splat_ps(v, 3);
    __m128 no_near_clip = _mm_cmple_ps(Z, W);

    z_all_in = _mm_and_ps(z_all_in, no_near_clip);

    xformed[i] = _mm_div_ps(v, W);

    screen_min = _mm_min_ps(screen_min, xformed[i]);
    screen_max = _mm_max_ps(screen_max, xformed[i]);
  }

  // If the AABB is clipped by the near plane
  //   assume (conservatively) that it's visible
  if(_mm_movemask_ps(z_all_in) != 0b1111) return VisibilityMesh::Visible;

  screen_min = _mm_max_ps(screen_min, _mm_setr_ps(0.0f, 0.0f, 0.0f, -INFINITY));
  screen_max = _mm_min_ps(screen_max,
    _mm_setr_ps((float)SizeMinusOne.x, (float)SizeMinusOne.y, 1.0f, INFINITY)
  );

  // Trivial rejection test
  if(_mm_movemask_ps(_mm_cmplt_ps(screen_max, screen_min))) return VisibilityMesh::Invisible;

  __m128 minmax_xy = _mm_shuffle_ps(screen_min, screen_max, _MM_SHUFFLE(1, 0, 1, 0));
  __m128i minmax_xyi = _mm_cvtps_epi32(minmax_xy);
  __m128i minmax_xyis = _mm_srai_epi32(minmax_xyi, 3);

  __m128 max_z = splat_ps(screen_max, 2);

  int rx0 = minmax_xyis.m128i_i32[0];
  int ry0 = minmax_xyis.m128i_i32[1];
  int rx1 = minmax_xyis.m128i_i32[2];
  int ry1 = minmax_xyis.m128i_i32[3];

  __m128 any_closer = _mm_setzero_ps();
  for(int by = ry0; by <= ry1; by++) {
    const auto *src_row = m_fb_coarse.get() + by*CoarseSize.x;

    for(int bx = rx0; bx <= rx1; bx++) {
      __m128 fb_max = _mm_load_ss(&src_row[bx].y);   // m_fb_coarse[by,bx] = vec2(min, max)

      any_closer = _mm_or_ps(any_closer, _mm_cmpnlt_ss(max_z, fb_max));
    }

    if(_mm_movemask_ps(any_closer)) return VisibilityMesh::Unknown;
  }

  // We can only reach here if the entire AABB is behind
  //   the contents of the framebuffer
  return VisibilityMesh::Invisible;
}

INTRIN_INLINE static void gather_aabb_verts(VisMesh4Tris out[3], uint idx, const __m128 xformed[])
{
  for(uint i = 0; i < 3; i++) {
    uint off = idx*3 + (2 - i);  // Reverse winding

    uint ind0 = BBoxIndices[off + 0];
    uint ind1 = BBoxIndices[off + 3];
    uint ind2 = BBoxIndices[off + 6];
    uint ind3 = BBoxIndices[off + 9];

    __m128 v0 = xformed[ind0];
    __m128 v1 = xformed[ind1];
    __m128 v2 = xformed[ind2];
    __m128 v3 = xformed[ind3];
    _MM_TRANSPOSE4_PS(v0, v1, v2, v3);  // Convert to SoA

    out[i].v[0] = v0;
    out[i].v[1] = v1;
    out[i].v[2] = v2;
    out[i].v[3] = v3;
  }
}

bool OcclusionBuffer::fullTest(VisibilityMesh& mesh, const mat4& mvp, void *xformed_in)
{
  auto fb = m_fb.get();

  // See rasterizeTile() for notes on how this works
  for(uint i = 0; i < NumAABBTris; i += NumSIMDLanes) {
    VisMesh4Tris xformed[3];
    gather_aabb_verts(xformed, i, (const __m128 *)xformed_in);

    __m128i fxx[3], fxy[3];
    for(uint m = 0; m < 3; m++) {
      fxx[m] = _mm_cvtps_epi32(xformed[m].v[0]);
      fxy[m] = _mm_cvtps_epi32(xformed[m].v[1]);
    }

    __m128i A0 = _mm_sub_epi32(fxy[1], fxy[2]);
    __m128i A1 = _mm_sub_epi32(fxy[2], fxy[0]);
    __m128i A2 = _mm_sub_epi32(fxy[0], fxy[1]);

    __m128i B0 = _mm_sub_epi32(fxx[2], fxx[1]);
    __m128i B1 = _mm_sub_epi32(fxx[0], fxx[2]);
    __m128i B2 = _mm_sub_epi32(fxx[1], fxx[0]);

    __m128i C0 = _mm_sub_epi32(mullo_epi32(fxx[1], fxy[2]), mullo_epi32(fxx[2], fxy[1]));
    __m128i C1 = _mm_sub_epi32(mullo_epi32(fxx[2], fxy[0]), mullo_epi32(fxx[0], fxy[2]));
    __m128i C2 = _mm_sub_epi32(mullo_epi32(fxx[0], fxy[1]), mullo_epi32(fxx[1], fxy[0]));

    __m128i area = mullo_epi32(B2, A1);
    area = _mm_sub_epi32(area, mullo_epi32(B1, A2));

    __m128 inv_area = _mm_rcp_ps(_mm_cvtepi32_ps(area));

    __m128 Z[3];
    Z[0] = xformed[0].v[2];
    Z[1] = _mm_mul_ps(_mm_sub_ps(xformed[1].v[2], Z[0]), inv_area);
    Z[2] = _mm_mul_ps(_mm_sub_ps(xformed[2].v[2], Z[0]), inv_area);

    __m128i startx, endx, starty, endy;
    tri_bbox(fxx, fxy, ivec2::zero(), SizeMinusOne, startx, starty, endx, endy);

    startx = _mm_and_si128(startx, _mm_set1_epi32(0xFFFFFFFE));
    starty = _mm_and_si128(starty, _mm_set1_epi32(0xFFFFFFFE));

    for(uint lane = 0; lane < NumSIMDLanes; lane++) {
      if(area.m128i_i32[lane] <= 0) continue;  // Skip back-facing triangles

      __m128 zz[3];
      for(uint vv = 0; vv < 3; vv++) zz[vv] = _mm_set1_ps(Z[vv].m128_f32[lane]);

      int startxx = startx.m128i_i32[lane];
      int startxy = starty.m128i_i32[lane];
      int endxx   = endx.m128i_i32[lane];
      int endxy   = endy.m128i_i32[lane];

      __m128i aa0 = _mm_set1_epi32(A0.m128i_i32[lane]);
      __m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
      __m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

      __m128i bb0 = _mm_set1_epi32(B0.m128i_i32[lane]);
      __m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
      __m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

      __m128i aa0_inc = _mm_slli_epi32(aa0, 1);
      __m128i aa1_inc = _mm_slli_epi32(aa1, 1);
      __m128i aa2_inc = _mm_slli_epi32(aa2, 1);

      __m128i row, col;
      int row_idx = startxy*Size.x + 2*startxx;

      col = _mm_add_epi32(ColumnOffsets, _mm_set1_epi32(startxx));
      __m128i aa0_col = mullo_epi32(aa0, col);
      __m128i aa1_col = mullo_epi32(aa1, col);
      __m128i aa2_col = mullo_epi32(aa2, col);

      row = _mm_add_epi32(RowOffsets, _mm_set1_epi32(startxy));
      __m128i bb0_row = _mm_add_epi32(mullo_epi32(bb0, row), _mm_set1_epi32(C0.m128i_i32[lane]));
      __m128i bb1_row = _mm_add_epi32(mullo_epi32(bb1, row), _mm_set1_epi32(C1.m128i_i32[lane]));
      __m128i bb2_row = _mm_add_epi32(mullo_epi32(bb2, row), _mm_set1_epi32(C2.m128i_i32[lane]));

      __m128i sum0_row = _mm_add_epi32(aa0_col, bb0_row);
      __m128i sum1_row = _mm_add_epi32(aa1_col, bb1_row);
      __m128i sum2_row = _mm_add_epi32(aa2_col, bb2_row);

      __m128i bb0_inc = _mm_slli_epi32(bb0, 1);
      __m128i bb1_inc = _mm_slli_epi32(bb1, 1);
      __m128i bb2_inc = _mm_slli_epi32(bb2, 1);

      __m128 zx = _mm_mul_ps(_mm_cvtepi32_ps(aa1_inc), zz[1]);
      zx = _mm_add_ps(zx, _mm_mul_ps(_mm_cvtepi32_ps(aa2_inc), zz[2]));

      for(int r = startxy; r < endxy; r += 2) {
        int idx = row_idx;
        __m128i alpha = sum0_row;
        __m128i beta  = sum1_row;
        __m128i gama  = sum2_row;

        __m128 betaf = _mm_cvtepi32_ps(beta);
        __m128 gamaf = _mm_cvtepi32_ps(gama);

        __m128 depth = zz[0];
        depth = _mm_add_ps(depth, _mm_mul_ps(betaf, zz[1]));
        depth = _mm_add_ps(depth, _mm_mul_ps(gamaf, zz[2]));

        __m128i any_out = _mm_setzero_si128();
        for(int c = startxx; c < endxx; c += 2) {
          __m128i mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);

          // Check if the contents of the framebuffer occlude the triangle
          __m128 prev_depth  = _mm_load_ps(fb + idx);
          __m128 depth_mask  = _mm_cmpge_ps(depth, prev_depth);
          __m128i final_mask = _mm_andnot_si128(mask, _mm_castps_si128(depth_mask));

          any_out = _mm_or_si128(any_out, final_mask);

          idx += 4;    // Computing 4 pixels at a time
          alpha = _mm_add_epi32(alpha, aa0_inc);
          beta  = _mm_add_epi32(beta, aa1_inc);
          gama  = _mm_add_epi32(gama, aa2_inc);
          depth = _mm_add_ps(depth, zx);
        }

        // Early out if any pixel is in front of the framebuffer
        if(!testz_si128(any_out, _mm_set1_epi32(0x80000000))) return true;

        row_idx += 2*Size.x;   // Advance to the next quad
        sum0_row = _mm_add_epi32(sum0_row, bb0_inc);
        sum1_row = _mm_add_epi32(sum1_row, bb1_inc);
        sum2_row = _mm_add_epi32(sum2_row, bb2_inc);
      }  // Each row
    }  // Each column
  }  // Each AABB triangle

  return false;
}

void OcclusionBuffer::binTriangles(const VisibilityMesh& mesh, uint object_id, uint mesh_id)
{
  auto num_triangles = mesh.numTriangles();
#if defined(NO_OCCLUSION_SSE)
  static constexpr uint TriIncrement = 1;
#else
  int num_lanes = NumSIMDLanes;
  int lane_mask = (1<<num_lanes) - 1;
  static constexpr uint TriIncrement = NumSIMDLanes;   // Work on NumSIMDLanes triangles at a time
#endif
  for(uint tri = 0; tri < num_triangles; tri += TriIncrement) {
#if defined(NO_OCCLUSION_SSE)
    auto xformed = mesh.gatherTri(tri);

    ivec2 fx[3];
    for(uint i = 0; i < 3; i++) {
      auto pt = xformed[i];
      fx[i] = { (int)(pt.x + 0.5f), (int)(pt.y + 0.5f) };
    }

    int area = tri_area(fx);

    // Compute triangles screen space bounding box in pixels
    auto [start, end] = tri_bbox(fx, ivec2::zero(), SizeMinusOne);

    // Skip triangle if area is 0
    if(area <= 0) continue;
    // Don't bin clipped traingles
    if(end.x < start.x || end.y < start.y) continue;

    // Reject triangles clipped by the near plane
    if(xformed[0].w <= 0.0f || xformed[1].w <= 0.0f || xformed[2].w <= 0.0f) continue;

    ivec2 startt = ivec2::max(start / TileSize, ivec2::zero());
    ivec2 endt   = ivec2::min(end / TileSize, SizeMinusOne);

    int row, col;
    for(row = startt.y; row <= endt.y; row++) {
      int off1 = Offset1.y * row;
      int off2 = Offset2.y * row;
      for(col = startt.x; col <= endt.x; col++) {
        int idx1 = off1 + (Offset1.x*col);
        int idx2 = off2 + (Offset2.x*col) + m_bin_counts[idx1];

        m_bin[idx2] = tri;
        m_obj_id[idx2]  = object_id;
        m_mesh_id[idx2] = mesh_id;

        m_bin_counts[idx1]++;
      }
    }
#else
    if(tri + NumSIMDLanes > num_triangles) {
      num_lanes = num_triangles - tri;
      lane_mask = (1<<num_lanes) - 1;
    }

    VisMesh4Tris xformed[3];
    mesh.gatherTri4(xformed, tri, num_lanes);

    // Convert X, Y to fixed point, not needed
    //    for Z, so avoid the extra work
    __m128i fxx[3], fxy[3];
    __m128i XY[3];
    __m128 Z[3];
    for(int i = 0; i < 3; i++) {
      fxx[i] = _mm_cvtps_epi32(xformed[i].v[0]);
      fxy[i] = _mm_cvtps_epi32(xformed[i].v[1]);

      // Interleave the X, Y coords
      __m128i inter0 = _mm_unpacklo_epi32(fxx[i], fxy[i]);
      __m128i inter1 = _mm_unpackhi_epi32(fxx[i], fxy[i]);

      XY[i] = _mm_packs_epi32(inter0, inter1);
      Z[i]  = xformed[i].v[2];
    }

    __m128i area = tri_area(fxx, fxy);   // Used to reject back-facing triangles
    __m128 inv_area = _mm_rcp_ps(_mm_cvtepi32_ps(area));

    // Setup Z-plane equation coefficients
    Z[1] = _mm_mul_ps(_mm_sub_ps(Z[1], Z[0]), inv_area);
    Z[2] = _mm_mul_ps(_mm_sub_ps(Z[2], Z[0]), inv_area);

    __m128i startx, endx, starty, endy;
    tri_bbox(fxx, fxy, ivec2::zero(), SizeMinusOne, startx, starty, endx, endy);

    // Generate active lane mask
    __m128i front = _mm_cmpgt_epi32(area, _mm_setzero_si128());
    __m128i non_emptyx = _mm_cmpgt_epi32(endx, startx);
    __m128i non_emptyy = _mm_cmpgt_epi32(endy, starty);
    __m128 accept1 = _mm_castsi128_ps(_mm_and_si128(_mm_and_si128(front, non_emptyx), non_emptyy));

    // Clip to the near plane
    __m128 W0 = _mm_cmpgt_ps(xformed[0].v[3], _mm_setzero_ps());
    __m128 W1 = _mm_cmpgt_ps(xformed[1].v[3], _mm_setzero_ps());
    __m128 W2 = _mm_cmpgt_ps(xformed[2].v[3], _mm_setzero_ps());

    // Reject back-facing, 0-area and near clipped triangles
    __m128 accept = _mm_and_ps(_mm_and_ps(accept1, W0), _mm_and_ps(W1, W2));
    uint tri_mask = _mm_movemask_ps(accept) & lane_mask;

    while(tri_mask) {   // Bin the non-rejected triangles
      int i = find_and_clear_lsb(&tri_mask);

      // Find tile extents of the triangle
      int startxx = std::max(startx.m128i_i32[i]/TileSize.x, 0);
      int endxx   = std::min(endx.m128i_i32[i]/TileSize.x, SizeInTiles.x-1);

      int startxy = std::max(starty.m128i_i32[i]/TileSize.y, 0);
      int endxy   = std::min(endy.m128i_i32[i]/TileSize.y, SizeInTiles.y-1);

      // Add the triangle to the bins which it's bbox covers
      int row, col;
      for(row = startxy; row <= endxy; row++) {
        int off1 = Offset1.y * row;
        int off2 = Offset2.y * row;
        for(col = startxx; col <= endxx; col++) {
          int idx1 = off1 + (Offset1.x*col);
          int idx2 = off2 + (Offset2.x*col) + m_bin_counts[idx1];
          BinnedTri *btri = m_bin.get() + idx2;

          btri->v[0].xy = XY[0].m128i_i32[i];
          btri->v[1].xy = XY[1].m128i_i32[i];
          btri->v[2].xy = XY[2].m128i_i32[i];

          btri->Z[0] = Z[0].m128_f32[i];
          btri->Z[1] = Z[1].m128_f32[i];
          btri->Z[2] = Z[2].m128_f32[i];

          m_bin_counts[idx1]++;
        }  // Each row
      }  // Each column
    }  // Each lane's triangle
#endif
  }  // Each triangle
}

void OcclusionBuffer::clearTile(ivec2 start, ivec2 end)
{
  float *fb = m_fb.get();
  int w = end.x - start.x;
#if defined(NO_OCCLUSION_SSE)
  for(int r = start.y; r < end.y; r++) {
    int row_idx = r*Size.x + start.x;
    memset(fb + row_idx, 0, w*sizeof(float));
  }
#else
  for(int r = start.y; r < end.y; r += 2) {
    int row_idx = r*Size.x + 2*start.x;
    memset(fb + row_idx, 0, 2*w*sizeof(float));
  }
#endif
}

void OcclusionBuffer::rasterizeTile(const std::vector<VisibilityObject *>& objects, uint tile_idx)
{
  auto fb = m_fb.get();

  uvec2 tile = { tile_idx % SizeInTiles.x, tile_idx / SizeInTiles.x };

  ivec2 tile_start = tile.cast<int>() * TileSize;
  ivec2 tile_end   = ivec2::min(tile_start + TileSize - ivec2(1, 1), Size - ivec2(1, 1));

  uint bin = 0,
    bin_idx = 0,
    off1 = Offset1.y*tile.y + Offset1.x*tile.x,
    off2 = Offset2.y*tile.y + Offset2.x*tile.x;

  uint num_tris = m_bin_counts[off1 + bin];

  clearTile(tile_start, tile_end);

#if defined(NO_OCCLUSION_SSE)
  VisibilityMesh::Triangle xformed;
#else
  intrin::set_flush_denormals_flush_to_zero();

  VisMesh4Tris gather_buf[2];
#endif
  bool done = false;
  bool all_empty = false;

  m_drawn_tris[tile_idx] = num_tris;
  while(!done) {
#if !defined (NO_OCCLUSION_SSE)
    int num_simd_tris = 0;
    for(uint lane = 0; lane < NumSIMDLanes; lane++) {   // Gather 4 transformed triangles
#endif
      while(num_tris <= 0) {
        bin++;
        if(bin >= 1) break;

        num_tris = m_bin_counts[off1 + bin];
        m_drawn_tris[tile_idx] += num_tris;
        bin_idx = 0;
      }

      if(!num_tris) break;
#if !defined (NO_OCCLUSION_SSE)
      const auto *btri = m_bin.get() + (off2 + bin*NumTrisPerBin + bin_idx);
      gather_buf[0].v[lane] = _mm_castsi128_ps(_mm_loadu_si128((const __m128i *)&btri->v[0].xy));
      gather_buf[1].v[lane] = _mm_castsi128_ps(_mm_loadl_epi64((const __m128i *)&btri->Z[1]));

      num_simd_tris++;
#else
      u16 object = m_obj_id[off2 + bin*NumTrisPerBin + bin_idx];
      u16 mesh   = m_mesh_id[off2 + bin*NumTrisPerBin + bin_idx];
      uint tri   = m_bin[off2 + bin*NumTrisPerBin + bin_idx];

      xformed = objects[object]->mesh(mesh).gatherTri(tri);
#endif
      all_empty = false;

      bin_idx++;
      num_tris--;
#if !defined(NO_OCCLUSION_SSE)
    }
#endif

    done = bin >= NumBins;
    if(all_empty) return;

#if defined(NO_OCCLUSION_SSE)
    ivec2 fx[3];
    float Z[3];
    for(uint i = 0; i < 3; i++) {
      fx[i] = vec2(xformed[i].x + 0.5f, xformed[i].y + 0.5f).cast<int>();
      Z[i] = xformed[i].z;
    }

    // Fab(x, y) =     Ax       +       By     +      C              = 0
    // Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
    // Compute A = (ya - yb) for the 3 line segments that make up each triangle
    int A0 = fx[1].y - fx[2].y;
    int A1 = fx[2].y - fx[0].y;
    int A2 = fx[0].y - fx[1].y;

    // Compute B = (xb - xa) for the 3 line segments that make up each triangle
    int B0 = fx[2].x - fx[1].x;
    int B1 = fx[0].x - fx[2].x;
    int B2 = fx[1].x - fx[0].x;

    // Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
    int C0 = fx[1].x * fx[2].y - fx[2].x * fx[1].y;
    int C1 = fx[2].x * fx[0].y - fx[0].x * fx[2].y;
    int C2 = fx[0].x * fx[1].y - fx[1].x * fx[0].y;

    int area = tri_area(fx);
    float inv_area = 1.0f / (float)area;

    Z[1] = (Z[1] - Z[0]) * inv_area;
    Z[2] = (Z[2] - Z[0]) * inv_area;

    auto [start, end] = tri_bbox(fx, tile_start, tile_end+1);

    start.x &= 0xFFFFFFFE; start.y &= 0xFFFFFFFE;

    int row_idx = start.y*Size.x + start.x;
    int col = start.x,
      row = start.y;

    int alpha0 = (A0 * col) + (B0 * row) + C0,
      beta0 = (A1 * col) + (B1 * row) + C1,
      gama0 = (A2 * col) + (B2 * row) + C2;

    float zx = A1*Z[1] + A2*Z[2];

    for(int r = start.y; r < end.y; r++) {
      int index = row_idx;
      int alpha = alpha0,
        beta = beta0,
        gama = gama0;

      float depth = Z[0] + Z[1]*beta + Z[2]*gama;

      for(int c = start.x; c < end.x; c++) {
        int mask = alpha | beta | gama;

        float prev_depth   = fb[index];
        float merged_depth = std::max(prev_depth, depth);
        float final_depth  = mask < 0 ? prev_depth : merged_depth;

        fb[index] = final_depth;

        index++;
        alpha += A0; beta += A1; gama += A2;
        depth += zx;
      }

      row++; row_idx += Size.x;
      alpha0 += B0; beta0 += B1; gama0 += B2;
    }
#else
    __m128i fxx[3], fxy[3];

    {
      // Transpose the coords:
      //   x0 x1 x2 x3        x0 y0 z0 w0
      //   y0 y1 y2 y3   =>   x1 y1 z1 w1
      //   z0 z1 z2 z3        x2 y2 z2 w2
      //   w0 w1 w2 w3        x3 y3 z3 w3
      __m128 v0 = gather_buf[0].v[0];
      __m128 v1 = gather_buf[0].v[1];
      __m128 v2 = gather_buf[0].v[2];
      __m128 v3 = gather_buf[0].v[3];
      _MM_TRANSPOSE4_PS(v0, v1, v2, v3);

      // Now v0, v1, v2 contain the vertices
      //   - v3 also contains Z[0] but we don't use it
      fxx[0] = _mm_srai_epi32(_mm_slli_epi32(_mm_castps_si128(v0), 16), 16);
      fxy[0] = _mm_srai_epi32(_mm_castps_si128(v0), 16);
      fxx[1] = _mm_srai_epi32(_mm_slli_epi32(_mm_castps_si128(v1), 16), 16);
      fxy[1] = _mm_srai_epi32(_mm_castps_si128(v1), 16);
      fxx[2] = _mm_srai_epi32(_mm_slli_epi32(_mm_castps_si128(v2), 16), 16);
      fxy[2] = _mm_srai_epi32(_mm_castps_si128(v2), 16);
    }

    // Fab(x, y) =     Ax       +       By     +      C              = 0
    // Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
    // Compute A = (ya - yb) for the 3 line segments that make up each triangle
    __m128i A0 = _mm_sub_epi32(fxy[1], fxy[2]);
    __m128i A1 = _mm_sub_epi32(fxy[2], fxy[0]);
    __m128i A2 = _mm_sub_epi32(fxy[0], fxy[1]);

    // Compute B = (xb - xa) for the 3 line segments that make up each triangle
    __m128i B0 = _mm_sub_epi32(fxx[2], fxx[1]);
    __m128i B1 = _mm_sub_epi32(fxx[0], fxx[2]);
    __m128i B2 = _mm_sub_epi32(fxx[1], fxx[0]);

    // Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
    __m128i C0 = _mm_sub_epi32(mullo_epi32(fxx[1], fxy[2]), mullo_epi32(fxx[2], fxy[1]));
    __m128i C1 = _mm_sub_epi32(mullo_epi32(fxx[2], fxy[0]), mullo_epi32(fxx[0], fxy[2]));
    __m128i C2 = _mm_sub_epi32(mullo_epi32(fxx[0], fxy[1]), mullo_epi32(fxx[1], fxy[0]));

    __m128i startx, endx, starty, endy;
    tri_bbox(fxx, fxy, tile_start, tile_end+1, startx, starty, endx, endy);

    startx = _mm_and_si128(startx, _mm_set1_epi32(0xFFFFFFFE));
    starty = _mm_and_si128(starty, _mm_set1_epi32(0xFFFFFFFE));

    // Now that we've set up 4 triangles, rasterize them one by one in 2x2 pixel quads
    for(int lane = 0; lane < num_simd_tris; lane++) {
      __m128 zz[] = {  // Z-plane equation coefficients
        splat_ps(gather_buf[0].v[lane], 3),
        splat_ps(gather_buf[1].v[lane], 0),
        splat_ps(gather_buf[1].v[lane], 1),
      };

      // Get the current triangles bounding box
      int startxx = startx.m128i_i32[lane];
      int endxx   = endx.m128i_i32[lane];
      int startxy = starty.m128i_i32[lane];
      int endxy   = endy.m128i_i32[lane];

      __m128i aa0 = _mm_set1_epi32(A0.m128i_i32[lane]);
      __m128i aa1 = _mm_set1_epi32(A1.m128i_i32[lane]);
      __m128i aa2 = _mm_set1_epi32(A2.m128i_i32[lane]);

      __m128i bb0 = _mm_set1_epi32(B0.m128i_i32[lane]);
      __m128i bb1 = _mm_set1_epi32(B1.m128i_i32[lane]);
      __m128i bb2 = _mm_set1_epi32(B2.m128i_i32[lane]);

      // Compute the horizontal barycentric coord deltas
      __m128i aa0_inc = _mm_slli_epi32(aa0, 1);
      __m128i aa1_inc = _mm_slli_epi32(aa1, 1);
      __m128i aa2_inc = _mm_slli_epi32(aa2, 1);

      __m128i row, col;   // Framebuffer row and column offsets

      // Compute the offset of the row where the quad will be stored.
      // The pixels of the quad are stored sequentially in memory like so:
      //    A B . .      becomes       A B C D
      //    C D . .                    . . . .
      int row_idx = startxy*Size.x + 2*startxx;

      col = _mm_add_epi32(ColumnOffsets, _mm_set1_epi32(startxx));
      __m128i aa0_col = mullo_epi32(aa0, col);
      __m128i aa1_col = mullo_epi32(aa1, col);
      __m128i aa2_col = mullo_epi32(aa2, col);

      row = _mm_add_epi32(RowOffsets, _mm_set1_epi32(startxy));
      __m128i bb0_row = _mm_add_epi32(mullo_epi32(bb0, row), _mm_set1_epi32(C0.m128i_i32[lane]));
      __m128i bb1_row = _mm_add_epi32(mullo_epi32(bb1, row), _mm_set1_epi32(C1.m128i_i32[lane]));
      __m128i bb2_row = _mm_add_epi32(mullo_epi32(bb2, row), _mm_set1_epi32(C2.m128i_i32[lane]));

      __m128i sum0_row = _mm_add_epi32(aa0_col, bb0_row);
      __m128i sum1_row = _mm_add_epi32(aa1_col, bb1_row);
      __m128i sum2_row = _mm_add_epi32(aa2_col, bb2_row);

      // Compute the vertical barycentric coord deltas
      __m128i bb0_inc = _mm_slli_epi32(bb0, 1);
      __m128i bb1_inc = _mm_slli_epi32(bb1, 1);
      __m128i bb2_inc = _mm_slli_epi32(bb2, 1);

      __m128 zx = _mm_mul_ps(_mm_cvtepi32_ps(aa1_inc), zz[1]);
      zx = _mm_add_ps(zx, _mm_mul_ps(_mm_cvtepi32_ps(aa2_inc), zz[2]));

      for(int r = startxy; r < endxy; r += 2) {
        // Barycentric coordinates
        int idx = row_idx;
        __m128i alpha = sum0_row;
        __m128i beta  = sum1_row;
        __m128i gama  = sum2_row;

        __m128 betaf = _mm_cvtepi32_ps(beta);
        __m128 gamaf = _mm_cvtepi32_ps(gama);

        // Compute depth from the barycentric coords
        __m128 depth = zz[0];
        depth = _mm_add_ps(depth, _mm_mul_ps(betaf, zz[1]));
        depth = _mm_add_ps(depth, _mm_mul_ps(gamaf, zz[2]));

        for(int c = startxx; c < endxx; c += 2) {
          // When alpha == beta == gama == 0 the pixel is outside the triangle
          __m128i mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);

          // Store the computed depth if it's > than what's in the framebuffer
          __m128 prev_depth   = _mm_load_ps(fb + idx);
          __m128 merged_depth = _mm_max_ps(depth, prev_depth);
          __m128 final_depth  = blendv_ps(merged_depth, prev_depth, _mm_castsi128_ps(mask));
          _mm_store_ps(fb + idx, final_depth);

          idx += 4;    // Computing 4 pixels at a time
          alpha = _mm_add_epi32(alpha, aa0_inc);
          beta  = _mm_add_epi32(beta, aa1_inc);
          gama  = _mm_add_epi32(gama, aa2_inc);
          depth = _mm_add_ps(depth, zx);
        }

        row_idx += 2*Size.x;   // Advance to the next quad
        sum0_row = _mm_add_epi32(sum0_row, bb0_inc);
        sum1_row = _mm_add_epi32(sum1_row, bb1_inc);
        sum2_row = _mm_add_epi32(sum2_row, bb2_inc);
      }   // Each row
    }   // Each lane's triangle
#endif
  }

#if !defined(NO_OCCLUSION_SSE)
  createCoarseTile(tile_start, tile_end+1);
#endif
}

void OcclusionBuffer::createCoarseTile(ivec2 tile_start, ivec2 tile_end)
{
#if defined(NO_OCCLUSION_SSE)
  assert(0 && "createCoarseTile() called with NO_OCCLUSION_SSE defined!");
#endif

  auto fb = m_fb.get();
  auto fb_coarse = m_fb_coarse.get();

  ivec2 s0 = tile_start / CoarseBlockSize,
    s1 = tile_end / CoarseBlockSize;

  for(int yt = s0.y; yt < s1.y; yt++) {
    const auto src_row = fb + (yt*CoarseBlockSize.y) * Size.x;
    auto dst_row       = fb_coarse + yt*CoarseSize.x;

    for(int xt = s0.x; xt < s1.x; xt++) {
      const auto src = src_row + (xt*CoarseBlockSize.x)*2;

      static constexpr std::array<int, 8> offsets = {
        0*Size.x, 0*Size.x + CoarseBlockSize.x,
        2*Size.x, 2*Size.x + CoarseBlockSize.x,
        4*Size.x, 4*Size.x + CoarseBlockSize.x,
        6*Size.x, 6*Size.x + CoarseBlockSize.x,
      };

      // Use starting values such that anything
      //   stored in the framebuffer will be
      //   smaller/bigger respectively
      __m128 min0 = _mm_set_ps1(1.0f);
      __m128 min1 = _mm_set_ps1(1.0f);

      __m128 max0 = _mm_setzero_ps();
      __m128 max1 = _mm_setzero_ps();

      for(int i = 0; i < offsets.size(); i++) {
        const auto src_quad = src + offsets[i];

        __m128 src_quad0 = _mm_load_ps(src_quad + 0);
        __m128 src_quad1 = _mm_load_ps(src_quad + NumSIMDLanes);

        // Find the min/max within the first 4 values of the block
        min0 = _mm_min_ps(min0, src_quad0); min1 = _mm_min_ps(min1, src_quad1);
        // Find the min/max within the second 4 values of the block
        max0 = _mm_max_ps(max0, src_quad0); max1 = _mm_max_ps(max1, src_quad1);
      }

      // Merge the minimums and maximums
      min0 = _mm_min_ps(min0, min1);
      max0 = _mm_max_ps(max0, max1);

      // Compute the horizontal min/max
      min0 = _mm_min_ps(min0, permute_ps(min0, _MM_SHUFFLE(1, 0, 3, 2)));
      min0 = _mm_min_ps(min0, permute_ps(min0, _MM_SHUFFLE(2, 3, 0, 1)));

      max0 = _mm_max_ps(max0, permute_ps(max0, _MM_SHUFFLE(1, 0, 3, 2)));
      max0 = _mm_max_ps(max0, permute_ps(max0, _MM_SHUFFLE(2, 3, 0, 1)));

      // minmax = min, max, _, _
      __m128 minmax = _mm_unpacklo_ps(max0, min0);

      // dst = vec2(min, max)
      _mm_storel_pi((__m64 *)(dst_row + xt), minmax);
    }  // Each row
  }  // Each column
}

}