#pragma once

#include <ek/euklid.h>
#include <ek/visobject.h>

#include <math/geometry.h>

#include <vector>
#include <memory>

// Uncomment the line below to disable the
//   use of SSE instructions in the rasterizer
//#define NO_OCCLUSION_SSE

namespace ek {

union XY {
  struct { u16 x, y; };

  u32 xy;
};

struct BinnedTri {
  XY v[3];     // Screen-space coords
  float Z[3];  // Plane equation
};

// See BinnedTrisPtr
void free_binnedtris(BinnedTri *btri);

// TODO: maybe replace the current implementation with Masked Occlusion Culling?
class OcclusionBuffer {
public:
  // Size of the underlying framebuffer
  //   - Can be adjusted
  static constexpr ivec2 Size     = { 320, 192 };
  // Size of a single binning tile
  //   - Must be adjusted to partition the framebuffer
  //     into an even number of tiles
  static constexpr ivec2 TileSize = { 40, 48 };

  static constexpr ivec2 SizeMinusOne = { Size.x - 1, Size.y - 1 };

  // Size of blocks of 'm_fb_coarse'
  //   - Do NOT change this
  static constexpr ivec2 CoarseBlockSize = { 8, 8 };
  static constexpr ivec2 CoarseSize = Size / CoarseBlockSize;

  // Number of tiles in framebuffer (rounded up)
  static constexpr ivec2 SizeInTiles = {
    (Size.x + TileSize.x-1)/TileSize.x,
    (Size.y + TileSize.y-1)/TileSize.y
  };

  enum {
    NumTrisPerBin = 1024*16,
    NumBins = SizeInTiles.area(),

    // When this value is exceeded bad things will happen...
    MaxTriangles = NumTrisPerBin * NumBins,

    NumSIMDLanes = 4,
  };

  static constexpr ivec2 Offset1 = { 1, SizeInTiles.x };
  static constexpr ivec2 Offset2 = { NumTrisPerBin, SizeInTiles.x * NumTrisPerBin };

  // Transforms a vector from clip space to viewport space
  //   and inverts the depth (from RH coordinate system to LH,
  //   which is more convinient for the rasterizer)
  static constexpr mat4 ViewportMatrix = {
    (float)Size.x * 0.5f,                  0.0f,  0.0f, (float)Size.x * 0.5f,
                    0.0f, (float)Size.y * -0.5f,  0.0f, (float)Size.y * 0.5f,
                    0.0f,                  0.0f, -1.0f,                 1.0f,
                    0.0f,                  0.0f,  0.0f,                 1.0f,
  };

  OcclusionBuffer();

  OcclusionBuffer& binTriangles(const std::vector<VisibilityObject *>& objects);

  OcclusionBuffer& rasterizeBinnedTriangles(const std::vector<VisibilityObject *>& objects);

  // Returns the framebuffer which can potentially be
  //   tile in 2x2 pixel quads
  const float *framebuffer() const;
  // Returns a copy of the framebuffer which has been
  //   detiled and flipped vertically
  std::unique_ptr<float[]> detiledFramebuffer() const;

  // Returns a framebuffer which stores vec2(min, max)
  //   for 8x8 blocks of the main framebuffer
  const vec2 *coarseFramebuffer() const;

  // Test the meshes AABB against the coarseFramebuffer()
  //   - When the return value == Visibility::Unknown
  //     fullTest() must be called to obtain a result
  VisibilityMesh::Visibility earlyTest(VisibilityMesh& mesh, const mat4& viewprojectionviewport,
    void /* __m128 */ *xformed_out);
  // Test the meshes AABB against the framebuffer()
  //   - Returns 'false' when the mesh is occluded
  bool fullTest(VisibilityMesh& mesh, const mat4& viewprojectionviewport,
    void /* __m128 */ *xformed_in);

private:
  void binTriangles(const VisibilityMesh& mesh, uint object_id, uint mesh_id);

  void clearTile(ivec2 start, ivec2 end);
  void rasterizeTile(const std::vector<VisibilityObject *>& objects, uint tile_idx);

  void createCoarseTile(ivec2 tile_start, ivec2 tile_end);

  // The framebuffer of size Size.area()
  std::unique_ptr<float[]> m_fb;
  // Stores vec2(min, max) for 8x8 blocks
  //   of the framebuffer 'm_fb'
  std::unique_ptr<vec2[]> m_fb_coarse;

#if defined(NO_OCCLUSION_SSE)
  // Stores SizeInTiles.area() * NumTrisPerBins entries
  std::unique_ptr<u16[]> m_obj_id;  // VisibilityObject index
  std::unique_ptr<u16[]> m_mesh_id; // VisibilityMesh index
  std::unique_ptr<uint[]> m_bin;    // Triangle index
#else
  // See note above VisibilityMesh::XformedPtr for rationale
  using BinnedTrisPtr = std::unique_ptr<BinnedTri[], decltype(&free_binnedtris)>;
  BinnedTrisPtr m_bin = { nullptr, nullptr };
#endif

  // Stores SizeInTiles.area() (number of bins) entries
  std::unique_ptr<u16[]> m_bin_counts;

  // Stores the number of rasterized triangles per bin
  std::unique_ptr<u16[]> m_drawn_tris;
};

}