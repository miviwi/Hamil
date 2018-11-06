#include <ui/uicommon.h>

namespace ui {

vec2 Geometry::pos() const
{
  return { x, y };
}

vec2 Geometry::size() const
{
  return { w, h };
}

Geometry Geometry::translate(vec2 t) const
{
  return {
    x+t.x, y+t.y,
    w, h
  };
}

Geometry Geometry::clip(const Geometry& g) const
{
  vec2 gb = { g.x+g.w, g.y+g.h };
  if(gb.x > (x+w) && gb.y > (y+h)) return Geometry{ 0, 0, 0, 0 };

  vec2 da = clamp(g.pos(), { x, y }, { x+w, y+h });
  vec2 db = clamp(gb, { x, y }, { x+w, y+h });

  return Geometry{ da.x, da.y, db.x-da.x, db.y-da.y };
}

Geometry Geometry::contract(vec2 factor) const
{
  return Geometry(
    x + factor.x, y + factor.y,
    w - 2.0f*factor.x, h - 2.0f*factor.y
  );
}

Geometry Geometry::contract(float factor) const
{
  return contract({ factor, factor });
}

Geometry Geometry::expand(vec2 factor) const
{
  return contract(-factor);
}

Geometry Geometry::expand(float factor) const
{
  return contract({ -factor, -factor });
}

bool Geometry::intersect(vec2 p) const
{
  vec2 a = { x, y },
    b = { x+w, y+h };

  return (p.x > a.x && p.x < b.x) && (p.y > a.y && p.y < b.y);
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
  auto f = [=](int v) -> byte
  {
    return (byte)clamp((int)(v-factor), 0, 255);
  };

  return Color(
    f(r), f(g), f(b),
    a
  );
}

Color Color::lighten(unsigned factor) const
{
  auto f = [=](unsigned v) -> byte
  {
    return (byte)clamp(v+factor, 0u, 255u);
  };

  return  Color(
    f(r), f(g), f(b),
    a
  );
}

Color Color::darkenf(double factor) const
{
  return darken((unsigned)(factor*255.0));
}

Color Color::lightenf(double factor) const
{
  return lighten((unsigned)(factor*255.0));
}

Color Color::luminance() const
{
  byte y = (byte)(r*0.2126 + g*0.7152 + b*0.0722);

  return Color{ y, y, y, a };
}

Color Color::opacity(double factor) const
{
  return Color{ r, g, b, (byte)(a*factor) };
}

vec4 Color::normalize() const
{
  return vec4{ r/255.0f, g/255.0f, b/255.0f, a/255.0f };
}

Position::Position() :
  Vector2<i16>(~0, ~0)
{
}

Position::Position(Vector2<i16> pos) :
  Vector2<i16>(pos)
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