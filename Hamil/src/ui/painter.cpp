#include <ui/painter.h>
#include <ui/drawable.h>

#include <cassert>
#include <cmath>
#include <cstring>

#include <iterator>
#include <unordered_map>

namespace ui {

const gx::VertexFormat VertexPainter::Fmt = 
  gx::VertexFormat()
    .attr(gx::i16, 2, gx::VertexFormat::UnNormalized)
    .attr(gx::u8, 4)
    .attrAlias(1, gx::u16, 2, gx::VertexFormat::UnNormalized)
  ;

Vertex::Vertex() :
  color(transparent())
{
}

Vertex::Vertex(vec2 pos_, Color color_) :
  pos(pos_), color(color_)
{
}

Vertex::Vertex(Position pos_, Color color_) :
  pos(pos_), color(color_)
{
}

VertexPainter::VertexPainter() :
  m_overlay(false),
  m_buf(nullptr), m_buf_rover(nullptr), m_buf_end(nullptr),
  m_ind(nullptr), m_ind_rover(nullptr), m_ind_end(nullptr)
{
  m_commands.reserve(32);
}

VertexPainter& VertexPainter::begin(Vertex *verts, size_t num_verts, u16 *inds, size_t num_inds)
{
  m_buf = m_buf_rover = verts;
  m_ind = m_ind_rover = inds;

  m_buf_end = m_buf + num_verts;
  m_ind_end = m_ind + num_inds;

  return *this;
}

VertexPainter& VertexPainter::line(vec2 a, vec2 b, float width, LineCap cap, float cap_r, Color ca, Color cb)
{
  if(b.x < a.x) {
    std::swap(a, b);
    std::swap(ca, cb);
  }

  a.x += 0.5f; b.x += 0.5f;

  float r = width/2.0f;
  auto n = line_normal(a, b);

  auto quad = [&,this](vec2 a, vec2 b, vec2 c, vec2 d, Color color)
  {
    auto offset = currentOffset();

    appendVertices({
     { a, color },
     { b, color },
     { c, color },
     { d, color }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      0, offset,
      4
    ));
  };

  auto d = n*r,
    u = (b-a).normalize()*cap_r;

  float alpha = acos(n.x);

  cap_r = cap_r > 0 ? cap_r : r;
  switch(cap) {
  case CapRound:  circleSegment(a, cap_r, alpha, alpha+PIf, ca, ca); break;
  case CapSquare: quad(a-u+d, a-u-d, a-d, a+d, ca); break;
  case CapButt:   break;
  }

  auto offset = currentOffset();

  appendVertices({
    { a+d, ca },
    { a-d, ca },
    { b-d, cb },
    { b+d, cb }
  });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    0, offset,
    4
  ));

  switch(cap) {
  case CapRound:  circleSegment(b, cap_r, alpha+PIf, alpha + 2.0f*PIf, cb, cb); break;
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

  auto n = line_normal(a, b);

  auto square_cap = [&, this](vec2 a, vec2 b, vec2 c, vec2 d, Color color)
  {
    auto offset = currentOffset();

    appendVertices({
      { a, color },
      { b, color },
      { c, color },
      { d, color }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      0, offset,
      4
    ));
  };

  auto d = n*r,
    u = (b-a).normalize()*cap_r;

  float alpha = acos(n.x);

  unsigned vertex_count = 0;

  cap_r = cap_r > 0 ? cap_r : r;
  switch(cap) {
  case CapRound:  arc(a, cap_r, alpha, alpha+PIf, ca); break;
  case CapSquare: square_cap(a-d, a-u-d, a-u+d, a+d, ca); break;
  case CapButt:   break;
  }

  auto offset = currentOffset();

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
    0, offset,
    4 + 1
  ));

  switch(cap) {
  case CapRound:  arc(b, cap_r, alpha+PIf, alpha + 2.0f*PIf, cb); break;
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
  if(g.size().isZero()) return *this;   // Nothing to paint

  auto offset = currentOffset();

  appendVertices({
    { { g.x, g.y, }, a },
    { { g.x, g.y+g.h, }, b },
    { { g.x+g.w, g.y+g.h, }, c },
    { { g.x+g.w, g.y, }, d }
  });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    0, offset,
    4
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
  auto offset = currentOffset();

  if(width <= 1.0f) {        // Fast path
    appendVertices({
      { { g.x, g.y, }, a },
      { { g.x, g.y+g.h, }, b },
      { { g.x+g.w, g.y+g.h, }, c },
      { { g.x+g.w, g.y, }, d }
    });

    appendCommand(Command::primitive(
      gx::LineLoop,
      0, offset,
      4
    ));
  } else {                   // Slow path...
    line(g.pos(),                  g.pos()+vec2{ 0,   g.h }, width, LineCap::CapButt, a, a);
    line(g.pos()+vec2{ 0,   g.h }, g.pos()+vec2{ g.w, g.h }, width, LineCap::CapButt, b, b);
    line(g.pos()+vec2{ g.w, g.h }, g.pos()+vec2{ g.w, 0   }, width, LineCap::CapButt, c, c);
    line(g.pos()+vec2{ g.w, 0   }, g.pos(),                  width, LineCap::CapButt, d, d);
  }

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
  auto offset = currentOffset();

  auto point = [=](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  appendVertices({ { pos, b } });

  float angle = start_angle;
  float step = (end_angle - start_angle)/(2.0f*radius);
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    appendVertices({ {point(angle), a } });

    angle += step;
    num_verts++;
  }

  appendVertices({ { point(end_angle), a } });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    0, offset,
    num_verts + 2
  ));

  return *this;
}

VertexPainter& VertexPainter::circle(vec2 pos, float radius, Color a, Color b)
{
  return circleSegment(pos, radius, 0, 2.0f*PIf + 0.001f, a, b);
}

VertexPainter& VertexPainter::circle(vec2 pos, float radius, Color c)
{
  return circle(pos, radius, c, c);
}

VertexPainter& VertexPainter::arc(vec2 pos, float radius, float start_angle, float end_angle, Color c)
{
  auto offset = currentOffset();

  auto point = [=](float angle) -> vec2
  {
    return vec2{
      pos.x + (radius * cos(angle)),
      pos.y + (radius * sin(angle)),
    };
  };

  float angle = start_angle;
  float step = radius < 100.0f ? PIf/16.0f : PIf/32.0f;
  unsigned num_verts = 0;
  while(angle <= end_angle) {
    appendVertices({ { point(angle), c } });

    angle += step;
    num_verts++;
  }

  appendVertices({ { point(end_angle), c } });

  appendCommand(Command::primitive(
    gx::LineStrip,
    0, offset,
    num_verts+1
  ));

  return *this;
}

VertexPainter& VertexPainter::arcFull(vec2 pos, float radius, Color c)
{
  return arc(pos, radius, 0, 2.0*PIf, c);
}

VertexPainter& VertexPainter::roundedRect(Geometry g, float radius, unsigned corners, Color a, Color b)
{
  auto quad = [this](float x, float y, float w, float h, Color a, Color b, Color c, Color d)
  {
    auto offset = currentOffset();

    appendVertices({
      { { x, y }, a },
      { { x, y+h }, b },
      { { x+w, y+h }, c },
      { { x+w, y }, d }
    });

    appendCommand(Command::primitive(
      gx::TriangleFan,
      0, offset,
      4
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
    circleSegment({ g.x+radius, g.y+radius }, radius, PIf, 3.0f*PIf/2.0f, b, a);
  if(corners & BottomLeft)
    circleSegment({ g.x+radius, g.y+radius + (g.h - d) }, radius, PIf/2.0f, PIf, b, a);
  if(corners & BottomRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius + (g.h - d) }, radius, 0, PIf/2.0f, b, a);
  if(corners & TopRight)
    circleSegment({ g.x+radius + (g.w - d), g.y+radius }, radius, 3.0f*PIf/2.0f, 2.0f*PIf, b, a);

  return *this;
}

VertexPainter& VertexPainter::roundedRect(Geometry g, float radius, unsigned corners, Color c)
{
  return roundedRect(g, radius, corners, c, c);
}

VertexPainter& VertexPainter::roundedBorder(Geometry g, float radius, unsigned corners, Color c)
{
  auto offset = currentOffset();
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
    float step = radius < 100.0f ? PIf/16.0f : PIf/32.0f;
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
    segment({ g.x+radius, g.y+radius }, PIf, 3.0f*PIf/2.0f);
  } else {
    appendVertices({ { { g.x, g.y }, c } });
    appendVertices({ { { g.x+radius, g.y }, c } });

    num_verts += 2;
  }

  if(corners & TopRight) {
    segment({ g.x + (g.w - radius), g.y+radius }, 3.0f*PIf/2.0f, PIf*2.0f);
  } else {
    appendVertices({ { { g.x + (g.w - radius), g.y }, c } });
    appendVertices({ { { g.x + g.w, g.y }, c } });
    appendVertices({ { { g.x + g.w, g.y + radius }, c } });

    num_verts += 3;
  }

  if(corners & BottomRight) {
    segment({ g.x + (g.w - radius), g.y + (g.h - radius) }, 0, PIf/2.0f);
  } else {
    appendVertices({ { { g.x + g.w, g.y + (g.h - radius) }, c } });
    appendVertices({ { { g.x + g.w, g.y + g.h }, c } });
    appendVertices({ { { g.x + (g.w - radius), g.y + g.h }, c } });

    num_verts += 3;
  }

  if(corners & BottomLeft) {
    segment({ g.x+radius, g.y + (g.h - radius) }, PIf/2.0f, PIf);
  } else {
    appendVertices({ { { g.x + radius, g.y + g.h }, c } });
    appendVertices({ { { g.x, g.y + g.h }, c } });
    appendVertices({ { { g.x, g.y + (g.h - radius) }, c } });

    num_verts += 3;
  }

  appendCommand(Command::primitive(
    gx::LineLoop,
    0, offset,
    num_verts
  ));

  return *this;
}

const float sqrt3 = sqrtf(3.0f);

VertexPainter& VertexPainter::triangle(vec2 pos, float h, float angle, Color color)
{
  auto offset = currentOffset();

  pos.x += 0.5f; pos.y += 0.5f;

  float s = sinf(angle);
  float c = cosf(angle);

  float a = 2.0f*h/sqrt3;
  float R = 2.0f*h/3.0f;
  float dx = a/2.0f;

  // Center at origin
  vec2 verts[3] = {
    { 0,   -R     },
    { dx,  R/2.0f },
    { -dx, R/2.0f },
  };

  // Rotate
  for(auto& v : verts) {
    v = {
      v.x*c - v.y*s,
      v.x*s + v.y*c
    };
  }

  appendVertices({
    { verts[0]+pos, color },
    { verts[1]+pos, color },
    { verts[2]+pos, color },
  });

  appendCommand(Command::primitive(
    gx::TriangleFan,
    0, offset,
    3
  ));

  return *this;
}

ft::String VertexPainter::appendTextVertices(ft::Font& font, const std::string& str)
{
  auto base = currentBase();
  auto offset = currentOffset();

  Vertex *ptr = m_buf + base;

  StridePtr<ft::Position> pos_ptr(&ptr->pos, sizeof(Vertex));
  StridePtr<ft::UV> uv_ptr(&ptr->uv, sizeof(Vertex));

  auto s = font.writeVertsAndIndices(str.data(), pos_ptr, uv_ptr, m_ind+offset);

  unsigned num = s.num();

  // Move the rovers forwards by the number of written vertices/indices
  m_buf_rover += (num / ft::NumCharIndices * ft::NumCharVerts);
  m_ind_rover += num;

  return s;
}

vec2 VertexPainter::textAlignCenter(ft::Font& font, Geometry g, vec2 text_size) const
{
  // The height is calculated so that we offset the text by only
  //   font.descender() if it's one line tall, otherwise we
  //   need to offset it by half of it's height as well.
  // Seems to work ok (?)
  float height = text_size.y - font.height();

  vec2 center = g.center();
  vec2 pos = {
    center.x - text_size.x/2.0f,
    center.y - (font.descender() + height/2.0f)
  };

  return pos.floor();
}

vec2 VertexPainter::textAlignLeft(ft::Font& font, Geometry g, vec2 text_size) const
{
  vec2 center = g.center();
  vec2 pos = {
    g.x,
    center.y - font.descender()
  };

  return pos.floor();
}

void VertexPainter::assertBegin()
{
  assert(m_buf && m_ind && "Attempted to paint without first calling begin()!");
}

void VertexPainter::checkEnoughSpace(size_t sz)
{
  if(m_buf_rover+sz < m_buf_end && m_ind_rover+sz < m_ind_end) return;

  throw NotEnoughSpaceError();
}

VertexPainter& VertexPainter::text(ft::Font& font, const std::string& str, vec2 pos, Color c)
{
  auto base = currentBase();
  auto offset = currentOffset();

  auto s = appendTextVertices(font, str);

  appendCommand(Command::text(
    font,
    pos.floor(), c,
    base, offset,
    s.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::textCentered(ft::Font& font, const std::string& str, Geometry g, Color c)
{
  auto base = currentBase();
  auto offset = currentOffset();

  auto s = appendTextVertices(font, str);

  appendCommand(Command::text(
    font,
    textAlignCenter(font, g, { s.width(), s.height() }), c,
    base, offset,
    s.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::textLeft(ft::Font& font, const std::string& str, Geometry g, Color c)
{
  auto base = currentBase();
  auto offset = currentOffset();

  auto s = appendTextVertices(font, str);

  appendCommand(Command::text(
    font,
    textAlignLeft(font, g, { s.width(), s.height() }), c,
    base, offset,
    s.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::drawable(const Drawable& drawable, vec2 pos)
{
  switch(drawable.type()) {
  case Drawable::Invalid: break; // Drawable wasn't initialized - nothing to draw

  case Drawable::Text:  return text(drawable, pos);
  case Drawable::Image: return image(drawable, pos);
  }

  return *this;
}

VertexPainter& VertexPainter::drawableCentered(const Drawable& drawable, Geometry g)
{
  switch(drawable.type()) {
  case Drawable::Invalid: break; // Drawable wasn't initialized - nothing to draw

  case Drawable::Text:  return textCentered(drawable, g);
  case Drawable::Image: return imageCentered(drawable, g);
  }

  return *this;
}

VertexPainter& VertexPainter::drawableLeft(const Drawable& drawable, Geometry g)
{
  switch(drawable.type()) {
  case Drawable::Invalid: break; // Drawable wasn't initialized - nothing to draw

  case Drawable::Text:  return textLeft(drawable, g);
  case Drawable::Image: return imageLeft(drawable, g);
  }

  return *this;
}

static void assert_text(const Drawable& text)
{
  assert(text.type() == Drawable::Text && "non-text Drawable passed to VertexPainter::text()!");
}

VertexPainter& VertexPainter::text(const Drawable& text, vec2 pos)
{
  assert_text(text);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(text);

  appendCommand(Command::text(
    text.textFont(),
    pos.floor(), text.textColor(),
    base, offset,
    text.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::textCentered(const Drawable& text, Geometry g)
{
  assert_text(text);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(text);

  vec2 center = g.center();

  appendCommand(Command::text(
    text.textFont(),
    textAlignCenter(text.textFont(), g, text.size()), text.textColor(),
    base, offset,
    text.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::textLeft(const Drawable& text, Geometry g)
{
  assert_text(text);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(text);

  appendCommand(Command::text(
    text.textFont(),
    textAlignLeft(text.textFont(), g, text.size()), text.textColor(),
    base, offset,
    text.num()
  ));

  return *this;
}

static void assert_image(const Drawable& image)
{
  assert(image.type() == Drawable::Image && "non-image Drawable passed to VertexPainter::image()!");
}

VertexPainter& VertexPainter::image(const Drawable& image, vec2 pos)
{
  assert_image(image);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(image);

  appendCommand(Command::image(
    pos, image.imageAtlasPage(),
    base, offset, image.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::imageCentered(const Drawable& image, Geometry g)
{
  assert_image(image);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(image);

  vec2 center = g.center();
  vec2 pos = center - image.size()*0.5f;

  appendCommand(Command::image(
    pos, image.imageAtlasPage(),
    base, offset, image.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::imageLeft(const Drawable& image, Geometry g)
{
  assert_image(image);

  auto base = currentBase();
  auto offset = currentOffset();

  appendVerticesAndIndices(image);

  vec2 center = g.center();
  vec2 pos = {
    g.x,
    center.y - image.size().y*0.5f
  };

  appendCommand(Command::image(
    pos, image.imageAtlasPage(),
    base, offset, image.num()
  ));

  return *this;
}

VertexPainter& VertexPainter::pipeline(const gx::Pipeline& pipeline)
{
  appendCommand(Command::switch_pipeline(pipeline));

  return *this;
}

VertexPainter& VertexPainter::beginOverlay()
{
  m_overlay = true;

  return *this;
}

VertexPainter& VertexPainter::endOverlay()
{
  m_overlay = false;

  return *this;
}

void VertexPainter::end()
{
  endOverlay();

  m_buf = m_buf_rover = m_buf_end = nullptr;
  m_ind = m_ind_rover = m_ind_end = nullptr;

  m_commands.clear();
  m_overlay_commands.clear();
}

size_t VertexPainter::currentBase() const
{
  return m_buf_rover - m_buf;
}

size_t VertexPainter::currentOffset() const
{
  return m_ind_rover - m_ind;
}

void VertexPainter::appendVertices(std::initializer_list<Vertex> verts)
{
  assertBegin();

  checkEnoughSpace(verts.size());

  auto ind = (u16)currentBase();

  for(const auto& v : verts) {
    *m_buf_rover++ = v;
    *m_ind_rover++ = ind++;
  }
}

void VertexPainter::appendVerticesAndIndices(const Drawable& d)
{
  checkEnoughSpace(d.num());

  d.appendVertices(&m_buf_rover, m_buf_end-m_buf_rover)
    .appendIndices(&m_ind_rover, m_ind_end-m_ind_rover);
}

void VertexPainter::restartPrimitive()
{
  *m_ind_rover++ = Vertex::RestartIndex;
}

void VertexPainter::appendCommand(const Command& c)
{
  if(m_overlay) {
    doAppendCommand(c, m_overlay_commands);
  } else {
    doAppendCommand(c, m_commands);
  }
}

void VertexPainter::doAppendCommand(const Command& c, std::vector<Command>& commands)
{
  if(commands.empty()) {
    commands.push_back(c);
    return;
  }

  Command& last = commands.back();

  // Try to merge command with previous if possible
  if(c.type == Primitive && last.type == c.type) {
    if(c.p == last.p) {
      last.num += c.num + 1;
    } else {
      commands.push_back(c);
    }
  } else {
    commands.push_back(c);
  }

  restartPrimitive();
}

}