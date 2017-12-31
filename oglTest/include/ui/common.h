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

  bool intersect(vec2 p) const;
  bool intersect(ivec2 p) const;

  vec2 center() const;
};

struct Color : public Vector4<byte> {
  Color(byte r_, byte g_, byte b_, byte a_) :
    Vector4<byte>(r_, g_, b_, a_)
  { }
  Color() :
    Vector4<byte>(0, 0, 0, 0)
  { }

  Color darken(unsigned factor) const;
  Color lighten(unsigned factor) const;
  Color luminance() const;
};

static Color transparent() { return Color{ 0, 0, 0, 0 }; }
static Color black() { return Color{ 0, 0, 0, 255 }; }
static Color white() { return Color{ 255, 255, 255, 255 }; }

}
