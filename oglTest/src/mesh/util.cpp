#include <mesh/util.h>

#include <cmath>

namespace mesh {

std::tuple<std::vector<PNVertex>, std::vector<u16>> sphere(uint rings, uint sectors)
{
  const auto R = 1.0f / (float)(rings-1);
  const auto S = 1.0f / (float)(sectors-1);

  std::vector<PNVertex> verts;
  verts.reserve(rings*sectors);
  for(uint r = 0; r < rings; r++) {
    for(uint s = 0; s < sectors; s++) {
      auto y = sin(-(PIf/2.0f) + PIf*r*R);
      auto x = cos(2.0f*PIf * s * S) * sin(PIf * r * R);
      auto z = sin(2.0f*PIf * s * S) * sin(PIf * r * R);

      verts.push_back({
        { x, y, z },
        { x, y, z }, 
      });
    }
  }


  std::vector<u16> inds;
  inds.reserve(rings*sectors * 6);
  for(uint r = 0; r < rings; r++) {
    for(uint s = 0; s < sectors; s++) {
      inds.push_back(r*sectors + s);
      inds.push_back((r+1)*sectors + s+1);
      inds.push_back(r*sectors + s+1);

      inds.push_back((r+1)*sectors + s+1);
      inds.push_back(r*sectors + s);
      inds.push_back((r+1)*sectors + s);
    }
  }

  return std::make_tuple(std::move(verts), std::move(inds));
}

}