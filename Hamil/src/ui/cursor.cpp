#include <ui/cursor.h>
#include <ui/uicommon.h>

#include <math/geometry.h>
#include <math/xform.h>
#include <math/util.h>

#include <uniforms.h>
#include <gx/gx.h>
#include <gx/context.h>
#include <gx/resourcepool.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/texture.h>
#include <gx/pipeline.h>
#include <gx/program.h>

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
    pool(4),
    buf(gx::BufferHandle::null()),
    pipeline(nullptr)
  {
    buf_id = pool.createBuffer<gx::VertexBuffer>("bvCursor",
        gx::Buffer::Static);

    buf = pool.getBuffer(buf_id);

    vtx_id = pool.create<gx::VertexArray>("aCursor",
        Fmt, buf.get<gx::VertexBuffer>());

    sampler_id = pool.create<gx::Sampler>(gx::Sampler::edgeclamp2d());

    tex_id = pool.createTexture<gx::Texture2D>("t2dCursor",
        gx::rg);

    tex = pool.getTexture(tex_id);

    program_id = pool.create<gx::Program>(gx::make_program(
         "pCursor",
         { vs_src }, { fs_src }, U.cursor
    ));

    pipeline = gx::Pipeline(&pool)
      .add<gx::Pipeline::VertexInput>([this](auto& vi) {
          vi.with_array(vtx_id);
      })
      .add<gx::Pipeline::InputAssembly>([](auto& ia) {
          ia.with_primitive(gx::Primitive::Triangles);
      })
      .add<gx::Pipeline::Scissor>([](auto& sc) {
          sc.no_test();
      })
      .add<gx::Pipeline::Blend>([](auto& b) {
          b.premult_alpha_blend();
      })
      .add<gx::Pipeline::Program>(program_id);
  }

  ~pCursorDriver()
  {
    // We have to release the handles manually as otherwise
    //   gx::~ResourcePool() could be called before their
    //   dtors (gx::~BufferHandle()/gx::~TextureHandle),
    //   which would be a bug, as no external reference to a
    //   ResourcePool's resource can exist before it's
    //   freed
    buf.release();
    tex.release();
  }

  gx::ResourcePool pool;

  static const gx::VertexFormat Fmt;
  gx::ResourcePool::Id buf_id = gx::ResourcePool::Invalid;
  gx::ResourcePool::Id vtx_id = gx::ResourcePool::Invalid;

  gx::ResourcePool::Id sampler_id = gx::ResourcePool::Invalid;
  gx::ResourcePool::Id tex_id     = gx::ResourcePool::Invalid;

  static const char *vs_src;
  static const char *fs_src;
  gx::ResourcePool::Id program_id = gx::ResourcePool::Invalid;

  gx::BufferHandle buf;
  //gx::VertexArray vtx;

  gx::TextureHandle tex;

  gx::Pipeline pipeline;
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

struct CursorUniforms : gx::Uniforms {
  Name cursor;

  mat4 uModelViewProjection;
  Sampler uTex;
};

const char *pCursorDriver::vs_src = R"VTX(
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

const char *pCursorDriver::fs_src = R"FRAG(
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

const gx::VertexFormat pCursorDriver::Fmt =
  gx::VertexFormat()
    .attr(gx::Type::i16, 2, gx::VertexFormat::UnNormalized)
    .attr(gx::Type::u16, 2, gx::VertexFormat::UnNormalized);

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

  auto vtx = generate_verts(p_cursors);
  p->buf().init(vtx.data(), vtx.size());

  auto tex = decode_texture(p_tex);
  p->tex().init(tex.get(), 0, TextureSize.s, TextureSize.t, gx::rg, gx::Type::u8);
  p->tex().swizzle(gx::Red, gx::Red, gx::Red, gx::Green);

  p->pool.get<gx::Program>(p->program_id).use()
    .uniformSampler(U.cursor.uTex, pCursorDriver::TexImageUnit);
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
  if(auto mouse = input->get<os::Mouse>()) {
    m_pos += { mouse->dx, mouse->dy };

    m_pos = clamp(m_pos, { }, FramebufferSize);
  }
}

void CursorDriver::paint()
{
  if(!m_shown) return;

  constexpr auto prim = gx::Primitive::Triangles;

  mat4 modelviewprojection = mat4::identity()
    *xform::ortho(0, 0, FramebufferSize.y, FramebufferSize.x, 0.0f, 1.0f)
    *xform::translate(m_pos)
    *xform::scale(1.0f);

  gx::ScopedPipeline sp(p->pipeline);

  auto& vtx = p->pool.get<gx::VertexArray>(p->vtx_id);

  gx::GLContext::current().texImageUnit(pCursorDriver::TexImageUnit)
      .bind(p->tex(), p->pool.get<gx::Sampler>(p->sampler_id));

  p->pool.get<gx::Program>(p->program_id).use()
    .uniformMatrix4x4(U.cursor.uModelViewProjection, modelviewprojection)
    .draw(prim, vtx, m_type*pCursorDriver::NumCursorVerts, pCursorDriver::NumCursorVerts);
}

void CursorDriver::type(Type type)
{
  m_type = type;
}

void CursorDriver::visible(bool visible)
{
  m_shown = visible;
}

bool CursorDriver::visible() const
{
  return m_shown;
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
