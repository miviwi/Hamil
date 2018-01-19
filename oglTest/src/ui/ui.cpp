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

uniform mat4 uModelViewProjection;

uniform bool uText;

const float fixed_factor = 1.0 / float(1<<4);

layout(location = 0) in vec2 iPos;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec2 iUV;

out VertexData {
  vec4 color;
  vec2 uv;
} output;

void main() {
  vec2 pos = iPos;
  if(!uText) pos *= fixed_factor;

  output.uv = iUV;
  output.color = iColor;

  gl_Position = uModelViewProjection * vec4(pos, 0, 1);
}

)VTX";

static const char *fs_src = R"FRAG(

uniform sampler2D uFontAtlas;

uniform bool uText;
uniform vec4 uTextColor;

in VertexData {
  vec4 color;
  vec2 uv;
} input;

out vec4 color;

void main() {
  vec4 font_sample = sampleFontAtlas(uFontAtlas, input.uv);

  if(uText) {
    color = uTextColor * font_sample;
  } else {
    color = input.color;
  }
}

)FRAG";

std::unique_ptr<gx::Program> ui_program;

void init()
{
  gx::Shader vtx(gx::Shader::Vertex, vs_src);
  gx::Shader frag(gx::Shader::Fragment, { ft::Font::frag_shader, fs_src });

  ui_program = std::make_unique<gx::Program>(vtx, frag);
  ui_program->getUniformsLocations(U::ui);
}

void finalize()
{
}

Ui::Ui(Geometry geom, const Style& style) :
  m_geom(geom), m_style(style), m_repaint(true),
  m_vtx(gx::Buffer::Dynamic), m_vtx_array(VertexPainter::Fmt, m_vtx),
  m_ind(gx::Buffer::Dynamic, gx::IndexBuffer::u16)
{
  enum {
    NumBufferElements = 256*1024
  };

  m_vtx.label("UI_vertex");
  m_vtx_array.label("UI_vertex_array");

  m_ind.label("UI_index");

  m_vtx.init(sizeof(Vertex), NumBufferElements);
  m_ind.init(sizeof(u16), NumBufferElements);
}

Ui::~Ui()
{
  for(const auto& frame : m_frames) delete frame;
}

ivec4 Ui::scissor_rect(Geometry g)
{
  auto ga = ivec2{ (int)g.x, (int)g.y },
    gb = ivec2{ ga.x+(int)g.w, ga.y+(int)g.h };

  return ivec4{ (int)g.x, (int)FramebufferSize.y - gb.y, (int)g.w, (int)g.h };
}

Ui& Ui::frame(Frame *frame)
{
  m_frames.push_back(frame);

  return *this;
}

void Ui::registerFrame(Frame *frame)
{
  if(frame->m_name) m_names.insert({ frame->m_name, frame });
}

Frame *Ui::getFrameByName(const std::string& name)
{
  auto f = m_names.find(name);
  return f != m_names.end() ? f->second : nullptr;
}

const Style& Ui::style() const
{
  return m_style;
}

bool Ui::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!m_geom.intersect(mouse_pos)) return false;

  for(auto iter = m_frames.crbegin(); iter != m_frames.crend(); iter++) {
    const auto& frame = *iter;
    if(frame->input(mouse_pos, input)) return true;
  }

  return false;
}

void Ui::paint()
{
  if(m_frames.empty()) return;

  auto pipeline = gx::Pipeline::current();

  if(m_repaint) {
    m_painter = VertexPainter();
    for(const auto& frame : m_frames) frame->paint(m_painter, m_geom);

    m_vtx.upload(m_painter.vertices(), 0, m_painter.numVertices());
    m_ind.upload(m_painter.indices(), 0, m_painter.numIndices());
  }

  auto projection = xform::ortho(0, 0, Ui::FramebufferSize.y, Ui::FramebufferSize.x, 0.0f, 1.0f);

  m_painter.doCommands([&,this](VertexPainter::Command cmd)
  {
    switch(cmd.type) {
    case VertexPainter::Primitive:
      ui_program->use()
        .uniformMatrix4x4(U::ui.uModelViewProjection, projection)
        .uniformBool(U::ui.uText, false)
        .uniformVector4(U::ui.uTextColor, { 0, 0, 0, 0 })
        .drawBaseVertex(cmd.p, m_vtx_array, m_ind, cmd.base, cmd.offset, cmd.num);
      break;

    case VertexPainter::Text:
      cmd.font->bindFontAltas();
      ui_program->use()
        .uniformMatrix4x4(U::ui.uModelViewProjection, projection*xform::translate(cmd.pos.x, cmd.pos.y, 0))
        .uniformBool(U::ui.uText, true)
        .uniformInt(U::ui.uFontAtlas, ft::TexImageUnit)
        .uniformVector4(U::ui.uTextColor, cmd.color.normalize())
        .drawBaseVertex(cmd.p, m_vtx_array, m_ind, cmd.base, cmd.offset, cmd.num);
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