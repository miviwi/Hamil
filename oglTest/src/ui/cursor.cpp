#include <ui/cursor.h>
#include <ui/uicommon.h>

#include <gx/gx.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/texture.h>
#include <gx/pipeline.h>
#include <gx/program.h>
#include <uniforms.h>

#include <array>
#include <memory>

namespace ui {

class pCursorDriver { 
public:
  enum {
    TexImageUnit = 14,
    TexPixelSize = 2,

    NumCursorVerts = 6,
  };

  pCursorDriver() :
    buf(gx::Buffer::Static),
    vtx(Fmt, buf),
    tex(gx::rg)
  { }

  static const gx::VertexFormat Fmt;

  gx::VertexBuffer buf;
  gx::VertexArray vtx;

  gx::Sampler sampler;
  gx::Texture2D tex;
};

struct Vertex {
  Position pos;
  UV uv;
};

struct Cursor {
  u16 x, y, w, h;
};

static const Cursor p_cursors[] = {
  // CursorDriver::Default
  { 0, 0, 12, 19 },
};

static constexpr ivec2 TextureSize = { 16, 32 };

static const char *p_tex = 
  "X               "
  "XX              "
  "X.X             "
  "X..X            "
  "X...X           "
  "X....X          "
  "X.....X         "
  "X......X        "
  "X.......X       "
  "X........X      "
  "X.........X     "
  "X..........X    "
  "X......XXXXX    "
  "X...X..X        "
  "X..X X..X       "
  "X.X  X..X       "
  "XX    X..X      "
  "      X..X      "
  "       XX       "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
  "                "
;

static const char *vs_src = R"VTX(
uniform mat4 uModelViewProjection;

uniform sampler2D uTex;

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec2 iUV;

out VertexData {
  vec2 uv;
} vs_out;

void main() {
  ivec2 tex_sz = textureSize(uTex, 0);

  vs_out.uv = iUV / tex_sz;
  gl_Position = uModelViewProjection * vec4(iPosition, 0.0, 1.0);
}
)VTX";

static const char *fs_src = R"FRAG(
uniform sampler2D uTex;

in VertexData {
  vec2 uv;
} fs_in;

layout(location = 0) out vec4 color;

void main() {
  color = texture(uTex, fs_in.uv);
}
)FRAG";

std::unique_ptr<pCursorDriver> p;
std::unique_ptr<gx::Program> cursor_program;

const gx::VertexFormat pCursorDriver::Fmt =
  gx::VertexFormat()
    .attr(gx::i16, 2, gx::VertexFormat::UnNormalized)
    .attr(gx::u16, 2, gx::VertexFormat::UnNormalized);

const static auto pipeline =
  gx::Pipeline()
    .alphaBlend();

static std::vector<Vertex> generate_verts(const Cursor *cursors)
{
  auto make_vertex = [](ivec2 pos, ivec2 uv) -> Vertex
  {
    return Vertex{ pos.cast<i16>(), uv.cast<u16>() };
  };

  std::vector<Vertex> verts;
  verts.reserve(CursorDriver::NumTypes * pCursorDriver::NumCursorVerts);

  for(int i = 0; i < CursorDriver::NumTypes; i++) {
    auto cursor = cursors[i];

    verts.push_back(make_vertex({ 0, 0 },               { cursor.x, cursor.y }));
    verts.push_back(make_vertex({ 0, cursor.h },        { cursor.x, cursor.y+cursor.h }));
    verts.push_back(make_vertex({ cursor.w, cursor.h }, { cursor.x+cursor.w, cursor.y+cursor.h }));
    
    verts.push_back(make_vertex({ cursor.w, cursor.h }, { cursor.x+cursor.w, cursor.y+cursor.h }));
    verts.push_back(make_vertex({ 0, 0 },               { cursor.x, cursor.y }));
    verts.push_back(make_vertex({ cursor.w, 0 },        { cursor.x+cursor.w, cursor.y }));
  }

  return verts;
}

static std::unique_ptr<u8[]> decode_texture(const char *tex)
{
  auto ptr = new u8[TextureSize.s*TextureSize.t * pCursorDriver::TexPixelSize];

  for(int y = 0; y < TextureSize.t; y++) {
    auto src = (const u8 *)(tex + (y*TextureSize.s));
    auto dst = (u8 *)(ptr + (y*TextureSize.s * pCursorDriver::TexPixelSize));

    for(int x = 0; x < TextureSize.s; x++) {
      u16 pixel = 0;
      switch(*src++) {
      case 'X': pixel = 0xFFFF; break;
      case '.': pixel = 0x00FF; break;
      }

      *dst++ = pixel>>8;
      *dst++ = pixel&0xFF;
    }
  }

  return std::unique_ptr<u8[]>(ptr);
}

void CursorDriver::init()
{
  p = std::make_unique<pCursorDriver>();

  p->buf.label("CURSOR_vertex");
  p->vtx.label("CURSOR_vertex_array");

  auto vtx = generate_verts(p_cursors);
  p->buf.init(vtx.data(), vtx.size());

  p->sampler = gx::Sampler()
    .param(gx::Sampler::MinFilter, gx::Sampler::Linear)
    .param(gx::Sampler::MagFilter, gx::Sampler::Linear)
    .param(gx::Sampler::WrapS, gx::Sampler::EdgeClamp)
    .param(gx::Sampler::WrapT, gx::Sampler::EdgeClamp);

  auto tex = decode_texture(p_tex);
  p->tex.init(tex.get(), 0, TextureSize.s, TextureSize.t, gx::rg, gx::u8);
  p->tex.swizzle(gx::Red, gx::Red, gx::Red, gx::Green);

  p->tex.label("CURSOR_tex");

  gx::Shader vtx_shader(gx::Shader::Vertex, vs_src);
  gx::Shader frag_shader(gx::Shader::Fragment, fs_src);

  cursor_program = std::make_unique<gx::Program>(vtx_shader, frag_shader);
  cursor_program->getUniformsLocations(U::cursor);

  cursor_program->label("CURSOR_program");
}

void CursorDriver::finalize()
{
}

CursorDriver::CursorDriver(float x, float y) :
  m_type(Default), m_pos(x, y), m_shown(true)
{
}

void CursorDriver::input(const InputPtr& input)
{
  if(auto mouse = input->get<win32::Mouse>()) {
    m_pos += { mouse->dx, mouse->dy };

    m_pos = {
      std::max(0.0f, m_pos.x),
      std::max(0.0f, m_pos.y),
    };
  }
}

void CursorDriver::paint()
{
  if(!m_shown) return;

  mat4 modelviewprojection = xform::identity()
    *xform::ortho(0, 0, FramebufferSize.y, FramebufferSize.x, 0.0f, 1.0f)
    *xform::translate(m_pos)
    *xform::scale(1.0f);

  gx::ScopedPipeline sp(pipeline);

  gx::tex_unit(pCursorDriver::TexImageUnit, p->tex, p->sampler);
  cursor_program->use()
    .uniformMatrix4x4(U::cursor.uModelViewProjection, modelviewprojection)
    .uniformInt(U::cursor.uTex, pCursorDriver::TexImageUnit)
    .draw(gx::Triangles, p->vtx, m_type*pCursorDriver::NumCursorVerts, pCursorDriver::NumCursorVerts);
}

void CursorDriver::type(Type type)
{
  m_type = type;
}

void CursorDriver::visible(bool visible)
{
  m_shown = visible;
}

void CursorDriver::toggleVisible()
{
  m_shown = !m_shown;
}

vec2 CursorDriver::pos() const
{
  return m_pos;
}

ivec2 CursorDriver::ipos() const
{
  return ivec2( m_pos.x, m_pos.y );
}

}