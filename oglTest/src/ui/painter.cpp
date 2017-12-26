#include "ui/painter.h"

namespace ui {

const gx::VertexFormat VertexPainter::Fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::f32, 2)
    .attr(gx::VertexFormat::u8, 4);

VertexPainter& VertexPainter::rect(Geometry g, Color a, Color b, Color c, Color d)
{
  auto base = m_buf.size();

  Vertex vtx[] = {
    {
      { g.x, g.y, }, a
    },
    {
      { g.x, g.y+g.h, }, b
    },
    {
      { g.x+g.w, g.y+g.h, }, c
    },
    {
      { g.x+g.w, g.y, }, d
    }
  };

  m_buf.push_back(vtx[0]);
  m_buf.push_back(vtx[1]);
  m_buf.push_back(vtx[2]);

  m_buf.push_back(vtx[2]);
  m_buf.push_back(vtx[0]);
  m_buf.push_back(vtx[3]);

  m_commands.push_back({
    gx::Triangles,
    base, 6
  });

  return *this;
}

VertexPainter& VertexPainter::rect(Geometry g, Color c[])
{
  return rect(g, c[0], c[1], c[2], c[3]);
}

VertexPainter& VertexPainter::border(Geometry g, Color a, Color b, Color c, Color d)
{
  auto base = m_buf.size();

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

  m_commands.push_back({
    gx::LineLoop,
    base, 4
  });

  return *this;
}

VertexPainter& VertexPainter::border(Geometry g, Color c[])
{
  return border(g, c[0], c[1], c[2], c[3]);
}

VertexPainter& VertexPainter::circleSegment(vec2 pos, float radius,
                                            float start_angle, float end_angle, Color a, Color b)
{
  auto base = m_buf.size();

  m_buf.push_back({ pos, a });

  auto point = [&](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  float angle = start_angle;
  float step = 2.0f*PI/radius;
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    m_buf.push_back({ point(angle), b });

    angle += step;
    num_verts++;
  }

  m_buf.push_back({ point(end_angle), b });

  m_commands.push_back({
    gx::TriangleFan,
    base, num_verts+2
  });

  return *this;
}

VertexPainter& VertexPainter::circle(vec2 pos, float radius, Color a, Color b)
{
  return circleSegment(pos, radius, 0, 2.0*PI + 0.001, a, b);
}

void VertexPainter::uploadVerts(gx::VertexBuffer& buf)
{
  buf.init(m_buf.data(), m_buf.size());
}

}