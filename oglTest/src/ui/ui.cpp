#include "ui/ui.h"
#include "ui/painter.h"

#include "pipeline.h"
#include "buffer.h"
#include "vertex.h"
#include "program.h"
#include "uniforms.h"

#include <algorithm>
#include <memory>

namespace ui {

const vec2 Ui::FramebufferSize = { 1280.0f, 720.0f };

static const char *vs_src = R"VTX(
#version 330

uniform mat4 uProjection;

layout(location = 0) in vec2 iPos;
layout(location = 1) in vec4 iColor;

out VertexData {
  vec4 color;
} output;

void main() {
  output.color = iColor;
  gl_Position = uProjection * vec4(iPos, 0.0f, 1.0f);
}

)VTX";

static const char *fs_src = R"FRAG(
#version 330

in VertexData {
  vec4 color;
} input;

out vec4 color;

void main() {
  color = input.color;
}

)FRAG";

std::unique_ptr<gx::Program> ui_program;

void init()
{
  gx::Shader vtx(gx::Shader::Vertex, vs_src);
  gx::Shader frag(gx::Shader::Fragment, fs_src);

  ui_program = std::make_unique<gx::Program>(vtx, frag);
  ui_program->getUniformsLocations(U::ui);
}

void finalize()
{
}

Ui::Ui(Geometry geom) :
  m_geom(geom)
{
}

void Ui::frame(Frame *frame)
{
  m_frame = frame;
}

void Ui::paint()
{
  m_frame->paint(m_geom);
}

Frame::Frame(Geometry geom) :
  m_pos_mode(Local), m_geom(geom), m_border_width(1.0f)
{
  std::fill(m_color, m_color+4, Color{ 0, 0, 0, 0 });
  std::fill(m_border_color, m_border_color+4, Color{ 0, 0, 0, 0 });
}

Frame& Frame::color(Color a, Color b, Color c, Color d)
{
  m_color[0] = a;
  m_color[1] = b;
  m_color[2] = c;
  m_color[3] = d;

  return *this;
}

Frame& Frame::border(float width, Color a, Color b, Color c, Color d)
{
  m_border_width = width;
  m_border_color[0] = a;
  m_border_color[1] = b;
  m_border_color[2] = c;
  m_border_color[3] = d;

  return *this;
}

void Frame::paint(Geometry parent)
{
  Geometry g = parent.clip(m_geom);

  auto ga = vec2{ g.x, g.y },
    gb = vec2{ g.x+g.w, g.y+g.h };
  
  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(g.x-1, Ui::FramebufferSize.y - gb.y-1, g.w+1, g.h+1);
  gx::ScopedPipeline sp(pipeline);

  VertexPainter painter;

  vec2 circle = {
    (gb.x-ga.x)/2.0f + ga.x,
    (gb.y-ga.y)/2.0f + ga.y,
  };

  painter
    .rect(g, m_color)
    .border(g, m_border_color)
    .circleSegment(circle, 180.0f, 3.0f*PI/2.0f, 2.0f*PI, transparent(), white())
    .circle(circle, 20.0f, { 255, 0, 0, 255 }, { 255, 0, 255, 255 });

  gx::VertexBuffer vtx(gx::Buffer::Static);
  painter.uploadVerts(vtx);

  gx::VertexArray arr(VertexPainter::Fmt, vtx);

  auto projection = xform::ortho(0, 0, Ui::FramebufferSize.y, Ui::FramebufferSize.x, 0.0f, 1.0f);

  painter.doCommands([&](VertexPainter::Command cmd)
  {
    ui_program->use()
      .uniformMatrix4x4(U::ui.uProjection, projection)
      .draw(cmd.primitive, arr, cmd.offset, cmd.num);
  });
}

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

}