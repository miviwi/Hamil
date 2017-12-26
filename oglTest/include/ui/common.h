#pragma once

namespace ui {

struct Geometry {
  float x, y, w, h;

  Geometry clip(const Geometry& g) const;
};

using Color = Vector4<byte>;

static Color transparent() { return Color{ 0, 0, 0, 0 }; }
static Color white() { return Color{ 255, 255, 255, 255 }; }

}
