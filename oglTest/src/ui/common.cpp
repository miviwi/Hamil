#include "ui/common.h"

namespace ui {
Geometry Geometry::clip(const Geometry& g) const
{
  auto b = vec2{ x+w, w+h },
    gb = vec2{ g.x+g.w, g.y+g.h };

  if(gb.x > b.x || gb.y > b.y) return Geometry{ 0, 0, 0, 0 };

  vec2 da = {
    clamp(g.x, x, b.x),
    clamp(g.y, y, b.y),
  };

  vec2 db = {
    clamp(gb.x, x, b.x),
    clamp(gb.y, y, b.y),
  };

  return Geometry{ da.x, da.y, db.x-da.x, db.y-da.y };
}

Geometry Geometry::contract(float factor) const
{
  return Geometry(
    x + factor, y + factor,
    w - 2.0f*factor, h - 2.0f*factor
  );
}

}