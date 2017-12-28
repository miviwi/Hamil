#pragma once

#include <common.h>

#include "vmath.h"

namespace ui {

struct Geometry {
  float x, y, w, h;

  Geometry(float x_, float y_, float w_, float h_) :
    x(x_), y(y_), w(w_), h(h_)
  { }
  Geometry(vec2 pos, float w_, float h_) :
    x(pos.x), y(pos.y), w(w_), h(h_)
  { }

  Geometry clip(const Geometry& g) const;
  Geometry contract(float factor) const;
};

using Color = Vector4<byte>;

static Color transparent() { return Color{ 0, 0, 0, 0 }; }
static Color black() { return Color{ 0, 0, 0, 255 }; }
static Color white() { return Color{ 255, 255, 255, 255 }; }

}
