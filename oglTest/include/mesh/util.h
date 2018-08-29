#pragma once

#include <common.h>

#include <math/geometry.h>

#include <vector>
#include <tuple>

namespace mesh {

struct PNVertex {
  vec3 pos, normal;
};

struct PVertex {
  vec3 pos;

  PVertex(float x, float y, float z) :
    pos(x, y, z)
  { }
};

std::tuple<std::vector<PNVertex>, std::vector<u16>> sphere(uint h, uint v);

// Each argument represents the HALF-extent of a given edge
std::tuple<std::vector<PVertex>, std::vector<u16>> box(float half_w, float half_h, float half_d);

}

