#include "ui/painter.h"

#include <cmath>
#include <unordered_map>

namespace ui {

const gx::VertexFormat VertexPainter::Fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::i16, 2, false)
    .attr(gx::VertexFormat::u8, 4);

Vertex::Vertex() :
  color(transparent())
{
}

Vertex::Vertex(vec2 pos_, Color color_) :
  pos(pos_), color(color_)
{
}

VertexPainter::VertexPainter()
{
  m_buf.reserve(InitialBufferReserve);
  m_ind.reserve(InitialBufferReserve);
  m_commands.reserve(32);
}

VertexPainter& VertexPainter::line(vec2 a, vec2 b, float width, LineCap cap, float cap_r, Color ca, Color cb)
{
  if(b.x < a.x) {
    std::swap(a, b);
    std::swap(ca, cb);
  }

  float r = width/2.0f;
  auto n = vec2{ -(b.y - a.y), b.x - a.x }.normalize();

  auto quad = [&,this](vec2 a, vec2 b, vec2 c, vec2 d, Color color)
  {
    unsigned base = m_ind.size();

    appendVertices({
     { a, color },
     { b, color },
     { c, color },
     { d, color }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      base, 4
    ));
  };

  auto d = n*r,
    u = (b-a).normalize()*cap_r;

  float alpha = acos(n.x);

  cap_r = cap_r > 0 ? cap_r : r;
  switch(cap) {
  case CapRound:  circleSegment(a, cap_r, alpha, alpha+PI, ca, ca); break;
  case CapSquare: quad(a-u+d, a-u-d, a-d, a+d, ca); break;
  case CapButt:   break;
  }

  unsigned base = m_ind.size();

  appendVertices({
    { a+d, ca },
    { a-d, ca },
    { b-d, cb },
    { b+d, cb }
  });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    base, 4
  ));

  switch(cap) {
  case CapRound:  circleSegment(b, cap_r, alpha+PI, alpha + 2.0f*PI, cb, cb); break;
  case CapSquare: quad(b+d, b-d, b+u-d, b+u+d, cb); break;
  case CapButt:   break;
  }

  return *this;
}

VertexPainter& VertexPainter::line(vec2 a, vec2 b, float width, LineCap cap, Color ca, Color cb)
{
  return line(a, b, width, cap, 0, ca, cb);
}

VertexPainter& VertexPainter::lineBorder(vec2 a, vec2 b, float width,
                                         LineCap cap, float cap_r, Color ca, Color cb)
{
  if(b.x < a.x) {
    std::swap(a, b);
    std::swap(ca, cb);
  }

  float r = width/2.0f;

  auto n = vec2{ -(b.y - a.y), b.x - a.x }.normalize();

  auto square_cap = [&, this](vec2 a, vec2 b, vec2 c, vec2 d, Color color)
  {
    unsigned base = m_ind.size();

    appendVertices({
      { a, color },
      { b, color },
      { c, color },
      { d, color }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      base, 4
    ));
  };

  auto d = n*r,
    u = (b-a).normalize()*cap_r;

  float alpha = acos(n.x);

  unsigned vertex_count = 0;

  cap_r = cap_r > 0 ? cap_r : r;
  switch(cap) {
  case CapRound:  arc(a, cap_r, alpha, alpha+PI, ca); break;
  case CapSquare: square_cap(a-d, a-u-d, a-u+d, a+d, ca); break;
  case CapButt:   break;
  }

  unsigned base = m_ind.size();

  appendVertices({
    { a+d, ca },
    { b+d, cb }
  });
  
  restartPrimitive();

  appendVertices({
    { b-d, cb },
    { a-d, ca },
  });

  appendCommand(Command::primitive(
    gx::LineStrip,
    base, 4 + 1
  ));

  switch(cap) {
  case CapRound:  arc(b, cap_r, alpha+PI, alpha + 2.0f*PI, cb); break;
  case CapSquare: square_cap(b+d, b+u+d, b+u-d, b-d, cb); break;
  case CapButt:   break;
  }

  return *this;
}

VertexPainter& VertexPainter::lineBorder(vec2 a, vec2 b, float width, LineCap cap, Color ca, Color cb)
{
  return lineBorder(a, b, width, cap, 0, ca, cb);
}

VertexPainter& VertexPainter::rect(Geometry g, Color a, Color b, Color c, Color d)
{
  auto base = m_ind.size();

  appendVertices({
    { { g.x, g.y, }, a },
    { { g.x, g.y+g.h, }, b },
    { { g.x+g.w, g.y+g.h, }, c },
    { { g.x+g.w, g.y, }, d }
  });

  appendCommand(Command::primitive(
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

VertexPainter& VertexPainter::border(Geometry g, float width, Color a, Color b, Color c, Color d)
{
  auto base = m_ind.size();

  g.x += 0.5f; g.y += 0.5f;
  g.w -= 1.0f; g.h -= 1.0f;

  appendVertices({
    { { g.x, g.y, }, a },
    { { g.x, g.y+g.h, }, b },
    { { g.x+g.w, g.y+g.h, }, c },
    { { g.x+g.w, g.y, }, d }
  });

  appendCommand(Command::primitive(
    gx::LineLoop,
    base, 4
  ));

  return *this;
}

VertexPainter& VertexPainter::border(Geometry g, float width, const Color c[4])
{
  return border(g, width, c[0], c[1], c[2], c[3]);
}

VertexPainter& VertexPainter::border(Geometry g, float width, Color c)
{
  return border(g, width, c, c, c, c);
}

VertexPainter& VertexPainter::circleSegment(vec2 pos, float radius,
                                            float start_angle, float end_angle, Color a, Color b)
{
  auto base = m_ind.size();

  auto point = [=](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  appendVertices({ { pos, b } });

  float angle = start_angle;
  float step = (end_angle - start_angle)/radius;
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    appendVertices({ {point(angle), a } });

    angle += step;
    num_verts++;
  }

  appendVertices({ { point(end_angle), a } });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    base, num_verts + 2
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
  auto base = m_ind.size();

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
    appendVertices({ { point(angle), c } });

    angle += step;
    num_verts++;
  }

  appendVertices({ { point(end_angle), c } });

  appendCommand(Command::primitive(
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
  unsigned base = m_ind.size();

  auto quad = [this](float x, float y, float w, float h, Color a, Color b, Color c, Color d)
  {
    unsigned base = m_ind.size();

    appendVertices({
      { { x, y }, a },
      { { x, y+h }, b },
      { { x+w, y+h }, c },
      { { x+w, y }, d }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      base, 4
    ));
  };

  float d = 2.0f*radius;

  quad(g.x+radius, g.y, g.w - d, radius, a, b, b, a);
  quad(g.x, g.y+radius, radius, g.h - d, a, a, b, b);
  quad(g.x+radius, g.y+g.h-radius, g.w - d, radius, b, a, a, b);
  quad(g.x+g.w-radius, g.y+radius, radius, g.h - d, b, b, a, a);

  quad(g.x+radius, g.y+radius, g.w - d, g.h - d, b, b, b, b);

  if(~corners & TopLeft) {
    quad(g.x, g.y, radius, radius, a, a, b, a);
  }
  if(~corners & BottomLeft) {
    quad(g.x, g.y+g.h-radius, radius, radius, a, a, a, b);
  }
  if(~corners & BottomRight) {
    quad(g.x+g.w-radius, g.y+g.h-radius, radius, radius, b, a, a, a);
  }
  if(~corners & TopRight) {
    quad(g.x+g.w-radius, g.y, radius, radius, a, b, a, a);
  }

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
  unsigned base = m_ind.size();
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
      appendVertices({ { point(angle), c } });

      angle += step;
      num_verts++;
    }

    appendVertices({ { point(end_angle), c } });
    num_verts++;
  };

  float d = 2.0f*radius;

  g.x += 0.5f; g.y += 0.5f;
  g.w -= 1.0f; g.h -= 1.0f;

  if(corners & TopLeft) {
    segment({ g.x+radius, g.y+radius }, PI, 3.0f*PI/2.0f);
  } else {
    appendVertices({ { { g.x, g.y }, c } });
    appendVertices({ { { g.x+radius, g.y }, c } });

    num_verts += 2;
  }

  if(corners & TopRight) {
    segment({ g.x + (g.w - radius), g.y+radius }, 3.0f*PI/2.0f, PI*2.0f);
  } else {
    appendVertices({ { { g.x + (g.w - radius), g.y }, c } });
    appendVertices({ { { g.x + g.w, g.y }, c } });
    appendVertices({ { { g.x + g.w, g.y + radius }, c } });

    num_verts += 3;
  }

  if(corners & BottomRight) {
    segment({ g.x + (g.w - radius), g.y + (g.h - radius) }, 0, PI/2.0f);
  } else {
    appendVertices({ { { g.x + g.w, g.y + (g.h - radius) }, c } });
    appendVertices({ { { g.x + g.w, g.y + g.h }, c } });
    appendVertices({ { { g.x + (g.w - radius), g.y + g.h }, c } });

    num_verts += 3;
  }

  if(corners & BottomLeft) {
    segment({ g.x+radius, g.y + (g.h - radius) }, PI/2.0f, PI);
  } else {
    appendVertices({ { { g.x + radius, g.y + g.h }, c } });
    appendVertices({ { { g.x, g.y + g.h }, c } });
    appendVertices({ { { g.x, g.y + (g.h - radius) }, c } });

    num_verts += 3;
  }

  appendCommand(Command::primitive(
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

VertexPainter& VertexPainter::text(ft::Font& font, ft::String str, vec2 pos, Color c)
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

Vertex *VertexPainter::vertices()
{
  return m_buf.data();
}

size_t VertexPainter::numVertices()
{
  return m_buf.size();
}

u16 *VertexPainter::indices()
{
  return m_ind.data();
}

size_t VertexPainter::numIndices()
{
  return m_ind.size();
}

void VertexPainter::appendVertices(std::initializer_list<Vertex> verts)
{
  unsigned ind = m_buf.size();

  for(const auto& v : verts) {
    m_buf.push_back(v);
    m_ind.push_back(ind++);
  }
}

void VertexPainter::restartPrimitive()
{
  m_ind.push_back(0xFFFF);
}

void VertexPainter::appendCommand(const Command& c)
{
  if(m_commands.empty()) {
    m_commands.push_back(c);
    return;
  }

  Command& last = m_commands.back();

  if(c.type == Primitive && last.type == c.type) {
    if(c.p == last.p) {
      last.num += c.num + 1;
    } else {
      m_commands.push_back(c);
    }
  } else {
    m_commands.push_back(c);
  }

  restartPrimitive();
}

}