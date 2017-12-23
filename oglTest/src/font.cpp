#include "common.h"
#include "font.h"
#include "pipeline.h"
#include "program.h"

#include <stb_rect_pack/stb_rect_pack.h>

#include <cstdio>
#include <cassert>
#include <string>
#include <algorithm>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace ft {

class pGlyph {
public:
  pGlyph() : m(0) {}
  pGlyph(int ch, unsigned idx, FT_GlyphSlot slot);

  pGlyph(pGlyph&& other);
  pGlyph(const pGlyph& other) = delete;

  ~pGlyph();

private:
  friend class Font;

  int m_ch;
  unsigned m_idx;
  FT_Glyph m;
};

class pString {
public:
  pString() :
    vtx_buf(gx::Buffer::Static),
    vtx(fmt, vtx_buf),
    ind(gx::Buffer::Static, gx::IndexBuffer::u16)
  { }

  FT_Face m;

  gx::VertexBuffer vtx_buf;
  gx::VertexArray vtx;

  gx::IndexBuffer ind;

  unsigned num;

  static const gx::VertexFormat fmt;
};

FT_Library ft;

static const char *vs_src = R"VTX(
#version 330

uniform mat4 uModelViewProjection;

layout(location = 0) in vec2 iPos;
layout(location = 1) in vec2 iUV;

out VertexData {
  vec2 uv;
} output;

void main() {
  output.uv = iUV;
  gl_Position = uModelViewProjection * vec4(iPos, 0.0f, 1.0f);
}

)VTX";

static const char *fs_src = R"FRAG(
#version 330

uniform sampler2D uAtlas;
uniform vec4 uColor;

in VertexData {
  vec2 uv;
} input;

out vec4 color;

void main() {
  float a = texture(uAtlas, input.uv).r;
  
  color = uColor*a;
}

)FRAG";

std::unique_ptr<gx::Program> font_program;

const gx::VertexFormat pString::fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::f32, 2)
    .attr(gx::VertexFormat::f32, 2);

const static auto pipeline =
  gx::Pipeline()
    .alphaBlend();

void init()
{
  auto err = FT_Init_FreeType(&ft);

  assert(!err && "FreeType init error!");

  gx::Shader vtx(gx::Shader::Vertex, vs_src);
  gx::Shader frag(gx::Shader::Fragment, fs_src);

  font_program = std::make_unique<gx::Program>(vtx, frag);
  font_program->getUniformsLocations(U::font);
}

void finalize()
{
  auto err = FT_Done_FreeType(ft);

  assert(!err && "FreeType finalize error!");
}

Font::Font(const FontFamily& family, unsigned height) :
  m_atlas(gx::Texture2D::r8)
{
  auto err = FT_New_Face(ft, family.getPath(), 0, &m);
  if(err) {
    std::string err_message = "FreeType face creation error " + std::to_string(err) + "!";

    MessageBoxA(nullptr, err_message.c_str(), "FreeTyper error!", MB_OK);
    ExitProcess(-2);
  }
      
  FT_Set_Pixel_Sizes(m, 0, height);

  // Load glyphs (TODO!!)
  std::vector<pGlyph> glyphs;
  for(int i = 0x20; i < 0x7F; i++) {
    auto idx = FT_Get_Char_Index(m, i);

    FT_Load_Glyph(m, idx, FT_LOAD_DEFAULT);
    FT_Render_Glyph(m->glyph, FT_RENDER_MODE_NORMAL);

    glyphs.emplace_back(i, idx, m->glyph);
  }

  populateRenderData(glyphs);
}

Font::~Font()
{
}

String Font::string(const char *str)
{
  struct Vertex {
    vec2 pos;
    vec2 uv;
  };

  std::vector<Vertex> verts;
  std::vector<u16> indices;

  const char *begin = str;
  FT_Vector pen = { 0, 0 };
  while(*str) {
    const auto& g = getGlyphRenderData(*str);

    float x = (float)pen.x / (float)(1<<16),
      y = (float)pen.y / (float)(1<<16);

    x += g.left;
    y -= g.top;

    if(*str == '\n') {
      pen.x = 0;
      pen.y += m->size->metrics.height << 10;

      str++;
      continue;
    }

    if(str != begin && FT_HAS_KERNING(m)) {
      FT_Vector kern = { 0, 0 };
      const auto& a = getGlyphRenderData(*(str - 1));

      FT_Get_Kerning(m, a.idx, g.idx, FT_KERNING_DEFAULT, &kern);

      x += kern.x / (float)(1<<6);
      y += kern.y / (float)(1<<6);
    }

    auto base = verts.size();

    verts.push_back({
      { x, y },
      g.uvs[0]
    }); 
    verts.push_back({
      { x, y+g.height },
      g.uvs[1]
    });
    verts.push_back({
      { x+g.width, y },
      g.uvs[2]
    });
    verts.push_back({
      { x+g.width, y+g.height },
      g.uvs[3]
    });

    indices.push_back(base+0); indices.push_back(base+1); indices.push_back(base+2);
    indices.push_back(base+2); indices.push_back(base+1); indices.push_back(base+3);

    pen.x += g.advance.x;
    pen.y += g.advance.y;

    str++;
  }

  auto s = std::shared_ptr<pString>(new pString, [](auto p) {
    delete p;
  });

  s->m = m;

  s->vtx_buf.init(verts.data(), verts.size());
  s->ind.init(indices.data(), indices.size());

  s->num = indices.size()/3;

  return s;
}

void Font::draw(const String& str, vec2 pos, vec4 color)
{
  pString *p = str.get();

  assert(p->m == m && "Drawing string with wrong Font!");

  gx::tex_unit(0, m_atlas, m_sampler);

  mat4 mvp = xform::ortho(0.0f, 0.0f, 720.0f, 1280.0f, 0.0f, 1.0f)
    *xform::translate(pos.x, pos.y, 0.0f);

  pipeline.use();

  font_program->use()
    .uniformMatrix4x4(U::font.uModelViewProjection, mvp)
    .uniformInt(U::font.uAtlas, 0)
    .uniformVector4(U::font.uColor, color)
    .drawTraingles(p->vtx, p->ind, p->num);
}

void Font::draw(const char * str, vec2 pos, vec4 color)
{
  draw(string(str), pos, color);
}

void Font::draw(const String& str, vec2 pos, vec3 color)
{
  draw(str, pos, vec4{ color.r, color.g, color.b, 1.0f });
}

void Font::draw(const char *str, vec2 pos, vec3 color)
{
  draw(str, pos, vec4{ color.r, color.g, color.b, 1.0f });
}

void Font::populateRenderData(const std::vector<pGlyph>& glyphs)
{
  int total_area = 0;
  for(auto& glyph : glyphs) {
    auto g = (FT_BitmapGlyph)glyph.m;

    auto area = (g->bitmap.width+2) * (g->bitmap.rows+2);
    total_area += area;
  }

  int atlas_sz = pow2_round(sqrt(total_area));

  stbrp_context ctx;
  std::vector<stbrp_node> nodes(atlas_sz);

  stbrp_init_target(&ctx, atlas_sz, atlas_sz, nodes.data(), nodes.size());

  std::vector<stbrp_rect> rects;
  rects.reserve(glyphs.size());
  for(int i = 0; i < glyphs.size(); i++) {
    auto g = (FT_BitmapGlyph)glyphs[i].m;

    // Spacing arounmd glyphs
    auto w = (stbrp_coord)(g->bitmap.width + 2),
      h = (stbrp_coord)(g->bitmap.rows + 2);

    rects.push_back(stbrp_rect{
      i,
      w, h
    });
  }

  stbrp_pack_rects(&ctx, rects.data(), rects.size());

  std::vector<unsigned char> img(atlas_sz*atlas_sz);

  // Blit glyphs to atlas
  for(auto& r : rects) {
    auto& g = glyphs[r.id];
    auto bm = ((FT_BitmapGlyph)g.m)->bitmap;

    assert(r.was_packed && "not all glyphs packed into atlas!!!");

    for(unsigned y = 0; y < bm.rows; y++) {
      auto dsty = (atlas_sz - (r.y+y))-2;

      auto src = bm.buffer + (y*bm.pitch);
      auto dst = img.data() + (dsty*atlas_sz) + r.x + 1;

      for(unsigned x = 0; x < bm.width; x++) *dst++ = *src++;
    }
  }

  // Setup atla texture and sampler
  m_atlas.init(img.data(), 0, atlas_sz, atlas_sz, gx::Texture2D::r, gx::Texture2D::u8);

  m_sampler.param(gx::Sampler::MinFilter, gx::Sampler::Linear);
  m_sampler.param(gx::Sampler::MagFilter, gx::Sampler::Linear);

  m_sampler.param(gx::Sampler::WrapS, gx::Sampler::EdgeClamp);
  m_sampler.param(gx::Sampler::WrapT, gx::Sampler::EdgeClamp);

  // Populate render data
  float denom = atlas_sz;
  for(unsigned i = 0; i < glyphs.size(); i++) {
    const auto& r = rects[i];
    auto g = glyphs[i].m;
    auto bm = (FT_BitmapGlyph)g;

    float x0 = r.x+1, y0 = r.y-1,
      x1 = r.x+r.w-1, y1 = r.y+r.h+1;

    y0 = atlas_sz - y0;
    y1 = atlas_sz - y1;

    vec2 uvs[] = {
      { x0/denom, y0/denom }, { x0/denom, y1/denom },
      { x1/denom, y0/denom }, { x1/denom, y1/denom },
    };

    GlyphRenderData rd;

    rd.idx = glyphs[i].m_idx;
    rd.top = bm->top;
    rd.left = bm->left;
    rd.width = bm->bitmap.width;
    rd.height = bm->bitmap.rows;
    rd.advance = g->advance;

    rd.uvs[0] = uvs[0]; rd.uvs[1] = uvs[1];
    rd.uvs[2] = uvs[2]; rd.uvs[3] = uvs[3];

    m_render_data.push_back(rd);
  }

  // Populate glyph indices
  m_glyph_index.resize(CHAR_MAX+1);
  for(unsigned i = 0; i < glyphs.size(); i++) {
    const auto& g = glyphs[i];

    m_glyph_index[g.m_ch] = i;
  }
}

int Font::glyphIndex(int ch)
{
  return m_glyph_index[ch];
}

const Font::GlyphRenderData& Font::getGlyphRenderData(int ch)
{
  return m_render_data[glyphIndex(ch)];
}

pGlyph::pGlyph(int ch, unsigned idx, FT_GlyphSlot slot) :
  m_ch(ch), m_idx(idx)
{
  auto err = FT_Get_Glyph(slot, &m);

  assert(!err && "FreeType glyph get error!");
}

pGlyph::pGlyph(pGlyph&& other) :
  m_ch(other.m_ch), m_idx(other.m_idx), m(other.m)
{
  other.m_ch = 0;
  other.m_idx = 0;
  other.m = 0;
}

pGlyph::~pGlyph()
{
  if(m) FT_Done_Glyph(m);
}

static const std::unordered_map<std::string, std::string> family_to_path = {
  { "arial", "C:\\Windows\\Fonts\\arial.ttf" },
  { "times", "C:\\Windows\\Fonts\\times.ttf" },
  { "consola", "C:\\Windows\\Fonts\\consola.ttf" },
};

FontFamily::FontFamily(const char *name)
{
  auto path = family_to_path.find(name);
  m_path = path != family_to_path.end() ? path->second : "";
}

const char *FontFamily::getPath() const
{
  return m_path.c_str();
}

}