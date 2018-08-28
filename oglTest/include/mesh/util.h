#pragma once

#include <common.h>

#include <math/geometry.h>

#include <vector>
#include <tuple>

namespace mesh {

struct PNVertex {
  vec3 pos, normal;
};

std::tuple<std::vector<PNVertex>, std::vector<u16>> sphere(uint h, uint v);

}