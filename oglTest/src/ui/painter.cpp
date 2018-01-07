#include "ui/painter.h"

#include <cmath>

namespace ui {

const gx::VertexFormat VertexPainter::Fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::i16, 2, false)
    .attr(gx::VertexFormat::u8, 4);

Vertex::Vertex() :
  pos(~0, ~0), color(transparent())
{
}

Vertex::Vertex(vec2 pos_, Color color_) :
  color(color_)
{
  float x = pos_.x * (float)(1<<4),
    y = pos_.y * (float)(1<<4);

  x = floor(x+0.5f); y = floor(y+0.5f);
  pos = Position{ (i16)x, (i16)y };
}

VertexPainter::VertexPainter()
{
  m_buf.reserve(InitialBufferReserve);
  m_commands.reserve(32);
}

VertexPainter& VertexPainter::rect(Geometry g, Color a, Color b, Color c, Color d)
{
  auto base = m_buf.size();

  m_buf.push_back({ { g.x, g.y, }, a });
  m_buf.push_back({ { g.x, g.y+g.h, }, b });
  m_buf.push_back({ { g.x+g.w, g.y+g.h, }, c });
  m_buf.push_back({ { g.x+g.w, g.y, }, d });

  m_commands.push_back(Command::primitive(
    gx::TriangleFan,
    base, 4
  ));

  return *this;
}

VertexPainter& VertexPainter::rect(Geometry g, const Color c[4])
{
  return rect(g, c[0], c[1], c[2], c[3]);
}

VertexPainter& VertexPainter::rect(Geometry g, Color c)
{
  return rect(g, c, c, c, c);
}

VertexPainter& VertexPainter::border(Geometry g, Color a, Color b, Color c, Color d)
{
  auto base = m_buf.size();

  g.x += 0.5f; g.y += 0.5f;
  g.w -= 1.0f; g.h -= 1.0f;

  m_buf.push_back({
    { g.x, g.y, }, a
  });
  m_buf.push_back({
    { g.x, g.y+g.h, }, b
  });
  m_buf.push_back({
    { g.x+g.w, g.y+g.h, }, c
  });
  m_buf.push_back({
    { g.x+g.w, g.y, }, d
  });

  m_commands.push_back(Command::primitive(
    gx::LineLoop,
    base, 4
  ));

  return *this;
}

VertexPainter& VertexPainter::border(Geometry g, const Color c[4])
{
  return border(g, c[0], c[1], c[2], c[3]);
}

VertexPainter& VertexPainter::border(Geometry g, Color c)
{
  return border(g, c, c, c, c);
}

VertexPainter& VertexPainter::circleSegment(vec2 pos, float radius,
                                            float start_angle, float end_angle, Color a, Color b)
{
  auto base = m_buf.size();

  m_buf.push_back({ pos, a });

  auto point = [=](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  float angle = start_angle;
  float step = (end_angle - start_angle)/radius;
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    m_buf.push_back({ point(angle), b });

    angle += step;
    num_verts++;
  }

  m_buf.push_back({ point(end_angle), b });

  m_commands.push_back(Command::primitive(
    gx::TriangleFan,
    base, num_verts+2
  ));

  return *this;
}

VertexPainter& VertexPainter::circle(vec2 pos, float radius, Color a, Color b)
{
  return circleSegment(pos, radius, 0, 2.0*PI + 0.001, a, b);
}

VertexPainter& VertexPainter::circle(vec2 pos, float radius, Color c)
{
  return circle(pos, radius, c, c);
}

VertexPainter& VertexPainter::arc(vec2 pos, float radius, float start_angle, float end_angle, Color c)
{
  auto base = m_buf.size();

  auto point = [=](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  float angle = start_angle;
  float step = radius < 100.0f ? PI/16.0f : PI/32.0f;
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    m_buf.push_back({ point(angle), c });

    angle += step;
    num_verts++;
  }

  m_buf.push_back({ point(end_angle), c });

  m_commands.push_back(Command::primitive(
    gx::LineStrip,
    base, num_verts+1
  ));

  return *this;
}

VertexPainter& VertexPainter::arcFull(vec2 pos, float radius, Color c)
{
  return arc(pos, radius, 0, 2.0*PI, c);
}

VertexPainter& VertexPainter::roundedRect(Geometry g, float radius, unsigned corners, Color a, Color b)
{
  unsigned base = m_buf.size();

  auto push_rect = [this](float x, float y, float w, float h, Color a, Color b, Color c, Color d)
  {
    m_buf.push_back({ { x, y }, a });
    m_buf.push_back({ { x, y+h }, b });
    m_buf.push_back({ { x+w, y+h }, c });
    m_buf.push_back({ { x+w, y+h }, c });
    m_buf.push_back({ { x, y }, a });
    m_buf.push_back({ { x+w, y }, d });
  };

  float d = 2.0f*radius;

  push_rect(g.x+radius, g.y, g.w - d, radius, a, b, b, a);
  push_rect(g.x, g.y+radius, radius, g.h - d, a, a, b, b);
  push_rect(g.x+radius, g.y+g.h-radius, g.w - d, radius, b, a, a, b);
  push_rect(g.x+g.w-radius, g.y+radius, radius, g.h - d, b, b, a, a);

  push_rect(g.x+radius, g.y+radius, g.w - d, g.h - d, b, b, b, b);

  unsigned num_tris = 0;

  if(~corners & TopLeft) {
    push_rect(g.x, g.y, radius, radius, a, a, b, a);
    num_tris += 6;
  }
  if(~corners & BottomLeft) {
    push_rect(g.x, g.y+g.h-radius, radius, radius, a, a, a, b);
    num_tris += 6;
  }
  if(~corners & BottomRight) {
    push_rect(g.x+g.w-radius, g.y+g.h-radius, radius, radius, b, a, a, a);
    num_tris += 6;
  }
  if(~corners & TopRight) {
    push_rect(g.x+g.w-radius, g.y, radius, radius, a, b, a, a);
    num_tris += 6;
  }

  m_commands.push_back(Command::primitive(
    gx::Triangles,
    base, 6*5 + num_tris
  ));

  if(corners & TopLeft)
    circleSegment({ g.x+radius, g.y+radius }, radius, PI, 3.0f*PI/2.0f, b, a);
  if(corners & BottomLeft)
    circleSegment({ g.x+radius, g.y+radius + (g.h - d) }, radius, PI/2.0f, PI, b, a);
  if(corners & BottomRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius + (g.h - d) }, radius, 0, PI/2.0f, b, a);
  if(corners & TopRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius }, radius, 3.0f*PI/2.0f, 2.0f*PI, b, a);

  return *this;
}

VertexPainter& VertexPainter::roundedBorder(Geometry g, float radius, unsigned corners, Color c)
{
  unsigned base = m_buf.size();
  unsigned num_verts = 0;

  auto segment = [&,this](vec2 pos, float start_angle, float end_angle) {
    auto point = [=](float angle) -> vec2
    {
      return vec2{
        pos.x + (radius * cos(angle)),
        pos.y + (radius * sin(angle)),
      };
    };

    float angle = start_angle;
    float step = radius < 100.0f ? PI/16.0f : PI/32.0f;
    while(angle <= end_angle) {
      m_buf.push_back({ point(angle), c });

      angle += step;
      num_verts++;
    }

    m_buf.push_back({ point(end_angle), c });
    num_verts++;
  };

  float d = 2.0f*radius;

  g.x += 0.5f; g.y += 0.5f;
  g.w -= 1.0f; g.h -= 1.0f;

  if(corners & TopLeft) {
    segment({ g.x+radius, g.y+radius }, PI, 3.0f*PI/2.0f);
  } else {
    m_buf.push_back({ { g.x, g.y }, c });
    m_buf.push_back({ { g.x+radius, g.y }, c });

    num_verts += 2;
  }

  if(corners & TopRight) {
    segment({ g.x + (g.w - radius), g.y+radius }, 3.0f*PI/2.0f, PI*2.0f);
  } else {
    m_buf.push_back({ { g.x + (g.w - radius), g.y }, c });
    m_buf.push_back({ { g.x + g.w, g.y }, c });
    m_buf.push_back({ { g.x + g.w, g.y + radius }, c });

    num_verts += 3;
  }

  if(corners & BottomRight) {
    segment({ g.x + (g.w - radius), g.y + (g.h - radius) }, 0, PI/2.0f);
  } else {
    m_buf.push_back({ { g.x + g.w, g.y + (g.h - radius) }, c });
    m_buf.push_back({ { g.x + g.w, g.y + g.h }, c });
    m_buf.push_back({ { g.x + (g.w - radius), g.y + g.h }, c });

    num_verts += 3;
  }

  if(corners & BottomLeft) {
    segment({ g.x+radius, g.y + (g.h - radius) }, PI/2.0f, PI);
  } else {
    m_buf.push_back({ { g.x + radius, g.y + g.h }, c });
    m_buf.push_back({ { g.x, g.y + g.h }, c });
    m_buf.push_back({ { g.x, g.y + (g.h - radius) }, c });

    num_verts += 3;
  }

  m_commands.push_back(Command::primitive(
    gx::LineLoop,
    base, num_verts
  ));

  return *this;
}

VertexPainter& VertexPainter::text(ft::Font& font, const char *str, vec2 pos, Color c)
{
  m_commands.push_back(Command::text(
    &font, font.string(str),
    pos, c
  ));

  return *this;
}

VertexPainter& VertexPainter::text(ft::Font & font, ft::String str, vec2 pos, Color c)
{
  m_commands.push_back(Command::text(
    &font, str,
    pos, c
  ));

  return *this;
}

VertexPainter& VertexPainter::textCentered(ft::Font& font, ft::String str, Geometry g, Color c)
{
  vec2 center = g.center();
  vec2 text_pos = {
    center.x - floor(font.width(str)/2.0f),
    center.y + floor(font.bearingY()/2.0f)
  };

  return text(font, str, text_pos, c);
}

VertexPainter& VertexPainter::pipeline(const gx::Pipeline& pipeline)
{
  m_commands.push_back(Command::pipeline(pipeline));

  return *this;
}

void VertexPainter::uploadVerts(gx::VertexBuffer& buf)
{
  buf.init(m_buf.data(), m_buf.size());
}

}