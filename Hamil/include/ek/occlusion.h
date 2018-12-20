#pragma once

#include <ek/euklid.h>
#include <ek/visobject.h>

#include <math/geometry.h>

#include <vector>
#include <memory>

namespace ek {

class OcclusionBuffer {
public:
  // Size of the underlying framebuffer
  static constexpr ivec2 Size     = { 320, 192 };
  // Size of a single binning tile
  static constexpr ivec2 TileSize = { 40, 48 };

  // Number of tiles in framebuffer (rounded up)
  static constexpr ivec2 SizeInTiles = {
    (Size.x + TileSize.x-1)/TileSize.x,
    (Size.y + TileSize.y-1)/TileSize.y
  };

  enum {
    NumTrisPerBin = 1024*16,
    NumBins = SizeInTiles.area(),

    MaxTriangles = NumTrisPerBin * NumBins,
  };

  static constexpr ivec2 Offset1 = { 1, SizeInTiles.x };
  static constexpr ivec2 Offset2 = { NumTrisPerBin, SizeInTiles.x * NumTrisPerBin };

  static constexpr mat4 ViewportMatrix = {
    (float)Size.x * 0.5f,                  0.0f, 0.0f, (float)Size.x * 0.5f,
                    0.0f, (float)Size.y * -0.5f, 0.0f, (float)Size.y * 0.5f,
                    0.0f,                  0.0f, 1.0f,                 0.0f,
                    0.0f,                  0.0f, 0.0f,                 1.0f,
  };

  OcclusionBuffer();

  OcclusionBuffer& binTriangles(const std::vector<VisibilityObject::Ptr>& objects);

  OcclusionBuffer& rasterizeBinnedTriangles(const std::vector<VisibilityObject::Ptr>& objects);

  const float *framebuffer() const;

private:
  void binTriangles(const VisibilityMesh& mesh, uint object_id, uint mesh_id);

  void clearTile(ivec2 start, ivec2 end);
  void rasterizeTile(const std::vector<VisibilityObject::Ptr>& objects, uint tile_idx);

  // The framebuffer of size Size.area()
  std::unique_ptr<float[]> m_fb;

  // Stores SizeInTiles.area() * NumTrisPerBins entries
  std::unique_ptr<u16[]> m_obj_id;  // VisibilityObject index
  std::unique_ptr<u16[]> m_mesh_id; // VisibilityMesh index
  std::unique_ptr<uint[]> m_bin;    // Triangle index

  // Stores SizeInTiles.area() (number of bins) entries
  std::unique_ptr<u16[]> m_bin_counts;

  // Stores the number of rasterized triangles per bin
  std::unique_ptr<u16[]> m_drawn_tris;
};

}