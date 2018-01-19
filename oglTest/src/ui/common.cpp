#include "ui/common.h"

namespace ui {

Geometry Geometry::clip(const Geometry& g) const
{
  auto b = vec2{ x+w, w+h },
    gb = vec2{ g.x+g.w, g.y+g.h };

  if(gb.x > b.x && gb.y > b.y) return Geometry{ 0, 0, 0, 0 };

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

bool Geometry::intersect(vec2 p) const
{
  vec2 a = { x, y },
    b = { x+w, y+h };

  return (p.x > a.x && p.x < b.x) && (p.y > a.y && p.y < b.y);
}

bool Geometry::intersect(ivec2 p) const
{
  return intersect(vec2{ (float)p.x, (float)p.y });
}

vec2 Geometry::center() const
{
  vec2 a = { x, y },
    b = { x+w, y+h };

  return vec2(
    floor((b.x-a.x)/2.0f + a.x),
    floor((b.y-a.y)/2.0f + a.y)
  );
}

Color Color::darken(unsigned factor) const
{
  return Color(
    (byte)clamp((int)(r-factor), 0, 255),
    (byte)clamp((int)(g-factor), 0, 255),
    (byte)clamp((int)(b-factor), 0, 255),
    a
  );
}

Color Color::lighten(unsigned factor) const
{
  return  Color(
    (byte)clamp(r+factor, 0u, 255u),
    (byte)clamp(g+factor, 0u, 255u),
    (byte)clamp(b+factor, 0u, 255u),
    a
  );
}

Color Color::luminance() const
{
  byte y = (r+g+b)/3;

  return Color{ y, y, y, a };
}

vec4 Color::normalize() const
{
  return vec4{ r/255.0f, g/255.0f, b/255.0f, a/255.0f };
}

Position::Position() :
  Vector2<i16>(~0, ~0)
{
}

Position::Position(vec2 pos)
{
  float fx = pos.x * (float)(1<<4),
    fy = pos.y * (float)(1<<4);

  // Doesen't seem necessary (?)
  //fx = floor(fx+0.5f); fy = floor(fy+0.5f);

  x = (i16)fx; y = (i16)fy;
}

}