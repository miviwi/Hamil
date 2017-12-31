#include "ui/ui.h"
#include "ui/frame.h"
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

Ui::Ui(Geometry geom, const Style& style) :
  m_geom(geom), m_style(style)
{
}

Ui::~Ui()
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

const Style& Ui::style() const
{
  return m_style;
}

bool Ui::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!m_geom.intersect(mouse_pos)) return false;

  return m_frame ? m_frame->input(mouse_pos, input) : false;
}

void Ui::paint()
{
  if(!m_frame) return;

  VertexPainter painter;
  auto pipeline = gx::Pipeline::current();

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

  pipeline.use();
}

}