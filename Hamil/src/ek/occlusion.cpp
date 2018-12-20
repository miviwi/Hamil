#include <ek/occlusion.h>

#include <utility>

namespace ek {

static std::pair<ivec2, ivec2> tri_bbox(ivec2 fx[3], ivec2 min_extent, ivec2 max_extent)
{
  ivec2 start = ivec2::max(ivec2::min(ivec2::min(fx[0], fx[1]), fx[2]), min_extent);
  ivec2 end   = ivec2::min(ivec2::max(ivec2::max(fx[0], fx[1]), fx[2]), max_extent);

  return std::make_pair(start, end);
}

OcclusionBuffer::OcclusionBuffer()
{
  m_fb = std::make_unique<float[]>(Size.area());

  m_obj_id  = std::make_unique<u16[]>(MaxTriangles);
  m_mesh_id = std::make_unique<u16[]>(MaxTriangles);
  m_bin     = std::make_unique<uint[]>(MaxTriangles);

  m_bin_counts = std::make_unique<u16[]>(NumBins);
  m_drawn_tris = std::make_unique<u16[]>(NumBins);
}

OcclusionBuffer& OcclusionBuffer::binTriangles(const std::vector<VisibilityObject::Ptr>& objects)
{
  // Clear the bin triangle counts
  memset(m_bin_counts.get(), 0, NumBins * sizeof(decltype(m_bin_counts)::element_type));

  for(uint o = 0; o < objects.size(); o++) {
    const auto& obj = objects[o];
    uint num_meshes = obj->numMeshes();
    for(uint m = 0; m < num_meshes; m++) binTriangles(obj->mesh(m), o, m);
  }

  return *this;
}

OcclusionBuffer& OcclusionBuffer::rasterizeBinnedTriangles(const std::vector<VisibilityObject::Ptr>& objects)
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

void OcclusionBuffer::binTriangles(const VisibilityMesh& mesh, uint object_id, uint mesh_id)
{
  static constexpr ivec2 SizeMinusOne = { Size.x - 1, Size.y - 1 };

  auto num_triangles = mesh.numTriangles();
  for(uint tri = 0; tri < num_triangles; tri++) {
    auto xformed = mesh.gatherTri(tri);

    ivec2 fx[3];
    for(uint i = 0; i < 3; i++) {
      auto pt = xformed[i];
      fx[i] = { (int)(pt.x + 0.5f), (int)(pt.y + 0.5f) };
    }

    int area = (fx[1].x - fx[0].x)*(fx[2].y - fx[0].y) - (fx[0].x - fx[2].x)*(fx[0].y - fx[1].y);

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
  }

  printf("binned:\n");
  for(auto y = 0; y < SizeInTiles.y; y++) {
    for(auto x = 0; x < SizeInTiles.x; x++) {
      printf("%d ", m_bin_counts[y*SizeInTiles.x + x]);
    }

    printf("\n");
  }
  printf("\n");
}

void OcclusionBuffer::clearTile(ivec2 start, ivec2 end)
{
  float *fb = m_fb.get();
  int w = end.x - start.x;
  for(int r = start.y; r < end.y; r++) {
    int row_idx = r*Size.x + start.x;
    memset(fb + row_idx, 0, w*sizeof(float));
  }
}

void OcclusionBuffer::rasterizeTile(const std::vector<VisibilityObject::Ptr>& objects, uint tile_idx)
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

  VisibilityMesh::Triangle xformed;
  bool done = false;
  bool all_empty = false;

  m_drawn_tris[tile_idx] = num_tris;
  while(!done) {
    while(num_tris <= 0) {
      bin++;
      if(bin >= 1) break;

      num_tris = m_bin_counts[off1 + bin];
      m_drawn_tris[tile_idx] += num_tris;
      bin_idx = 0;
    }

    if(!num_tris) break;

    u16 object = m_obj_id[off2 + bin*NumTrisPerBin + bin_idx];
    u16 mesh   = m_mesh_id[off2 + bin*NumTrisPerBin + bin_idx];
    uint tri   = m_bin[off2 + bin*NumTrisPerBin + bin_idx];

    xformed = objects[object]->mesh(mesh).gatherTri(tri);
    all_empty = false;

    bin_idx++;
    num_tris--;

    done = bin >= NumBins;
    if(all_empty) return;

    ivec2 fx[3];
    float Z[3];
    for(uint i = 0; i < 3; i++) {
      fx[i] = vec2(xformed[i].x + 0.5f, xformed[i].y + 0.5f).cast<int>();
      Z[i] = xformed[i].z;
    }

    int A0 = fx[1].y - fx[2].y;
    int A1 = fx[2].y - fx[0].y;
    int A2 = fx[0].y - fx[1].y;

    int B0 = fx[2].x - fx[1].x;
    int B1 = fx[0].x - fx[2].x;
    int B2 = fx[1].x - fx[0].x;

    int C0 = fx[1].x * fx[2].y - fx[2].x * fx[1].y;
    int C1 = fx[2].x * fx[0].y - fx[0].x * fx[2].y;
    int C2 = fx[0].x * fx[1].y - fx[1].x * fx[0].y;

    int area = (fx[1].x - fx[0].x)*(fx[2].y - fx[0].y) - (fx[0].x - fx[2].x)*(fx[0].y - fx[1].y);
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
  }

  printf("rasterized:\n");
  for(auto y = 0; y < SizeInTiles.y; y++) {
    for(auto x = 0; x < SizeInTiles.x; x++) {
      printf("%d ", m_drawn_tris[y*SizeInTiles.x + x]);
    }

    printf("\n");
  }
  printf("\n");
}

}