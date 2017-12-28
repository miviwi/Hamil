#include "ui/ui.h"
#include "ui/painter.h"

#include "pipeline.h"
#include "buffer.h"
#include "vertex.h"
#include "program.h"
#include "uniforms.h"

#include <algorithm>
#include <memory>

#include <Windows.h>

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

std::unique_ptr<ft::Font> font;

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

ivec4 Ui::scissor_rect(Geometry g)
{
  auto ga = ivec2{ (int)g.x, (int)g.y },
    gb = ivec2{ ga.x+(int)g.w, ga.y+(int)g.h };

  return ivec4{ (int)g.x-1, (int)FramebufferSize.y - gb.y-1, (int)g.w+2, (int)g.h+2 };
}

void Ui::frame(Frame *frame)
{
  m_frame = frame;
}

void Ui::paint()
{
  VertexPainter painter;

  m_frame->paint(painter, m_geom);

  gx::VertexBuffer vtx(gx::Buffer::Static);
  painter.uploadVerts(vtx);

  gx::VertexArray arr(VertexPainter::Fmt, vtx);

  auto projection = xform::ortho(0, 0, Ui::FramebufferSize.y, Ui::FramebufferSize.x, 0.0f, 1.0f);

  painter.doCommands([&](VertexPainter::Command cmd)
  {
    switch(cmd.type) {
    case VertexPainter::Primitive:
      ui_program->use()
        .uniformMatrix4x4(U::ui.uProjection, projection)
        .draw(cmd.p, arr, cmd.offset, cmd.num);
      break;

    case VertexPainter::Text:
      cmd.font->draw(cmd.str, cmd.pos, cmd.color);
      break;

    case VertexPainter::Pipeline:
      cmd.pipel.use();
      break;

    default: break;
    }
  });
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

void Frame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = parent.clip(m_geom);

  auto ga = vec2{ g.x, g.y },
    gb = vec2{ g.x+g.w, g.y+g.h };
  
 vec2 circle = {
    (gb.x-ga.x)/2.0f + ga.x,
    (gb.y-ga.y)/2.0f + ga.y,
  };

  auto time = GetTickCount();
  auto anim_time = 10000;

  auto anim_factor = (float)(time % anim_time)/(float)anim_time;

  float radius = lerp(0.0f, 55.0f/2.0f, anim_factor);

  Color gray = { 100, 100, 100, 255 },
    dark_gray = { 200, 200, 200, 255 };
  Geometry bg_pos = { gb.x - 30.0f, ga.y, 30.0f, 30.0f };
  vec2 close_btn = {
    (bg_pos.w / 2.0f) + bg_pos.x,
    (bg_pos.h / 2.0f) + bg_pos.y,
  };

  Geometry round_rect = {
    g.x+15.0f, g.y+15.0f, 55.0f, 55.0f
  };

  if(!font) font = std::make_unique<ft::Font>(ft::FontFamily("times"), 32);

  vec2 title_pos = {
    g.x+5.0f, g.y+font->ascender()
  };
  
  const char *title = "Title";

  painter
    .pipeline(gx::Pipeline().alphaBlend().scissor(Ui::scissor_rect(g)))
    .rect(g, m_color)
    .border(g, m_border_color)
    .text(*font.get(), title, title_pos, white())
    .border({ g.x+5.0f, title_pos.y-32.0f-font->descener(),
            font->width(font->string(title)), font->height(font->string(title)) }, white(), white(), white(), white())
    .rect(bg_pos, gray)
    .circle(close_btn, 9, { 228, 80, 80, 255 })
    .text(*font.get(), "Hello!", circle, white())
    .circleSegment(circle, 180.0f, 3.0f*PI/2.0f, 2.0f*PI, transparent(), white())
    //.roundedRect(round_rect, radius, VertexPainter::All & ~VertexPainter::BottomRight, gray)
    //.roundedRect(round_rect.contract(1), radius, VertexPainter::All & ~VertexPainter::BottomRight, dark_gray)
    .arcFull(circle, radius, { 0, 128, 0, 255 });
}

}