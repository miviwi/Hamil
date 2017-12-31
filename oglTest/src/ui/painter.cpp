#include "ui/painter.h"

namespace ui {

const gx::VertexFormat VertexPainter::Fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::f32, 2)
    .attr(gx::VertexFormat::u8, 4);

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

  g.x += 0.5; g.y += 0.5f;

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

VertexPainter& VertexPainter::roundedRect(Geometry g, float radius, unsigned corners, Color c)
{
  unsigned base = m_buf.size();

  auto push_rect = [this](float x, float y, float w, float h, Color c)
  {
    m_buf.push_back({ { x, y }, c });
    m_buf.push_back({ { x, y+h }, c });
    m_buf.push_back({ { x+w, y+h }, c });
    m_buf.push_back({ { x+w, y+h }, c });
    m_buf.push_back({ { x, y }, c });
    m_buf.push_back({ { x+w, y }, c });
  };

  float d = 2.0f*radius;

  push_rect(g.x+radius, g.y, g.w - d, radius, c);
  push_rect(g.x, g.y+radius, radius, g.h - d, c);
  push_rect(g.x+radius, g.y+g.h-radius, g.w - d, radius, c);
  push_rect(g.x+g.w-radius, g.y+radius, radius, g.h - d, c);

  push_rect(g.x+radius, g.y+radius, g.w - d, g.h - d, c);

  unsigned num_tris = 0;

  if(~corners & TopLeft) {
    push_rect(g.x, g.y, radius, radius, c);
    num_tris += 6;
  }
  if(~corners & BottomLeft) {
    push_rect(g.x, g.y+g.h-radius, radius, radius, c);
    num_tris += 6;
  }
  if(~corners & BottomRight) {
    push_rect(g.x+g.w-radius, g.y+g.h-radius, radius, radius, c);
    num_tris += 6;
  }
  if(~corners & TopRight) {
    push_rect(g.x+g.w-radius, g.y, radius, radius, c);
    num_tris += 6;
  }

  m_commands.push_back(Command::primitive(
    gx::Triangles,
    base, 6*5 + num_tris
  ));

  if(corners&TopLeft)
    circleSegment({ g.x+radius, g.y+radius }, radius, PI, 3.0f*PI/2.0f, c, c);
  if(corners&BottomLeft)
    circleSegment({ g.x+radius, g.y+radius + (g.h - d) }, radius, PI/2.0f, PI, c, c);
  if(corners&BottomRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius + (g.h - d) }, radius, 0, PI/2.0f, c, c);
  if(corners&TopRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius }, radius, 3.0f*PI/2.0f, 2.0f*PI, c, c);

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