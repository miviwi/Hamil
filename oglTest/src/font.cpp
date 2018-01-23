#include "common.h"
#include "font.h"
#include "pipeline.h"
#include "program.h"
#include "uniforms.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

#include <stb_rect_pack/stb_rect_pack.h>

#include <cstdio>
#include <cassert>
#include <string>
#include <list>
#include <algorithm>
#include <utility>

#include <Windows.h>

namespace ft {

struct pFt {
public:
  enum {
    NumBufferChars = 256*1024,

    NumCharVerts = 4,
    NumCharIndices = 5,

    Error = ~0u,
  };

  pFt() :
    ptr(0),
    buf(gx::Buffer::Dynamic), vtx(fmt, buf),
    ind(gx::Buffer::Dynamic, gx::IndexBuffer::u16)
  { }

  unsigned alloc(unsigned num_chars);
  void dealloc(unsigned offset, unsigned num_chars);

  static const gx::VertexFormat fmt;

  gx::VertexBuffer buf;
  gx::VertexArray vtx;

  gx::IndexBuffer ind;

private:
  unsigned allocAtEnd(unsigned num_chars);
  void coalesce();

  unsigned ptr;
  std::list<std::pair<unsigned, unsigned>> free_list;
};

class pFace {
public:
  pFace(FT_Face f);
  ~pFace();

  operator FT_Face() const { return m; }
  FT_Face get() const { return m; }

private:
  FT_Face m;
};

class pGlyph {
public:
  pGlyph() : m(0) {}
  pGlyph(int ch, unsigned idx, FT_Glyph glyph);

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
  pString();
  pString(const pString& other) = delete;
  ~pString();

  FT_Face m;

  unsigned base, offset;
  unsigned num;

  float width;
  float height;
};

struct Vertex {
  Position pos;
  UV uv;

  Vertex() :
    pos(0, 0), uv(0, 0)
  { }
  Vertex(vec2 pos_, UV uv_) :
    pos((i16)pos_.x, (i16)pos_.y), uv(uv_)
  { }
};

FT_Library ft;

static const char *vs_src = R"VTX(

uniform sampler2D uAtlas;
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

const char *Font::frag_shader = R"FRAG(
const float font_gamma = 1.8f;

vec4 sampleFontAtlas(in sampler2D atlas, vec2 uv)
{
  ivec2 atlas_sz = textureSize(atlas, 0);
  float a = texture(atlas, uv / atlas_sz).r;
  a = pow(a, 1.0f/font_gamma);

  return vec4(a);
}

)FRAG";

static const char *fs_src = R"FRAG(

uniform sampler2D uAtlas;
uniform vec4 uColor;

in VertexData {
  vec2 uv;
} input;

out vec4 color;

void main() {
  color = uColor * sampleFontAtlas(uAtlas, input.uv);
}

)FRAG";

unsigned atlas_id = 0;

std::unique_ptr<pFt> p;
std::unique_ptr<gx::Program> font_program;

const gx::VertexFormat pFt::fmt = 
  gx::VertexFormat()
    .attr(gx::VertexFormat::i16, 2, false)
    .attr(gx::VertexFormat::u16, 2, false);

const static auto pipeline =
  gx::Pipeline()
    .currentScissor()
    .alphaBlend()
    .primitiveRestart(PrimitiveRestartIndex);

void init()
{
  auto err = FT_Init_FreeType(&ft);
  assert(!err && "FreeType init error!");

  p = std::make_unique<pFt>();

  p->buf.label("FT_vertex");
  p->vtx.label("FT_vertex_array");

  p->ind.label("FT_index");

  p->buf.init(sizeof(Vertex)*4, pFt::NumBufferChars);
  p->ind.init(sizeof(u16)*6, pFt::NumBufferChars);

  gx::Shader vtx(gx::Shader::Vertex, vs_src);
  gx::Shader frag(gx::Shader::Fragment, { Font::frag_shader, fs_src });

  font_program = std::make_unique<gx::Program>(vtx, frag);
  font_program->getUniformsLocations(U::font);
  
  font_program->label("FT_program");
}

void finalize()
{
  auto err = FT_Done_FreeType(ft);
  assert(!err && "FreeType finalize error!");

  ft = nullptr;
}

Font::Font(const FontFamily& family, unsigned height) :
  m_atlas(gx::Texture2D::r8)
{
  FT_Face face;
  auto err = FT_New_Face(ft, family.getPath(), 0, &face);
  if(err) {
    std::string err_message = "FreeType face creation error " + std::to_string(err) + "!";

    MessageBoxA(nullptr, err_message.c_str(), "FreeTyper error!", MB_OK);
    ExitProcess(-2);
  }

  m = new pFace(face);
  m_bearing_y = 0;
      
  FT_Set_Pixel_Sizes(*m, 0, height);

  FT_Stroker stroker;
  err = FT_Stroker_New(ft, &stroker);
  assert(!err && "FreeType stroker creation error!");

  FT_Stroker_Set(stroker, 0<<6, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

  // Load glyphs (TODO!!)
  std::vector<pGlyph> glyphs;
  for(int i = 0x20; i < 0x7F; i++) {
    auto idx = FT_Get_Char_Index(face, i);
    FT_Glyph glyph;

    FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);

    m_bearing_y = std::max(m_bearing_y, (float)(face->glyph->metrics.horiBearingY >> 6));

    FT_Get_Glyph(face->glyph, &glyph);
    //FT_Glyph_StrokeBorder(&glyph, stroker, false, true);
    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);

    glyphs.emplace_back(i, idx, glyph);
  }
  FT_Stroker_Done(stroker);

  populateRenderData(glyphs);
}

Font::~Font()
{
  delete m;
}

String Font::string(const char *str) const
{
  auto length = strlen(str);

  unsigned ptr = p->alloc(length); // Simple free list alloator
  assert(ptr != pFt::Error && "no more space in string vertex buffer!");

  std::vector<Vertex> vtx(length*NumCharVerts);
  std::vector<u16> inds(length*NumCharIndices);

  StridePtr<Position> pos(&vtx.data()->pos, sizeof(Vertex));
  StridePtr<UV> uv(&vtx.data()->uv, sizeof(Vertex));

  auto s = writeVertsAndIndices(str, pos, uv, inds.data());

  s->base = ptr*NumCharVerts; s->offset = ptr*NumCharIndices;

  p->buf.upload(vtx.data(), ptr*NumCharVerts, vtx.size());
  p->ind.upload(inds.data(), ptr*NumCharIndices, inds.size());

  return s;
}

String Font::writeVertsAndIndices(const char *str, StridePtr<Position> pos, StridePtr<UV> uv, u16 *inds) const
{
  auto make_pos = [](auto x, auto y) -> Position
  {
    return{ (i16)x, (i16)y };
  };

  FT_Face face = *m;

  unsigned num_chars = 0;

  const char *begin = str;
  FT_Vector pen = { 0, 0 };
  float width = 0.0f;
  while(*str) {
    const auto& g = getGlyphRenderData(*str);

    float x = (float)pen.x / (float)(1<<16),
      y = (float)pen.y / (float)(1<<16);

    x += g.left;
    y -= g.top;

    if(*str == '\n') {
      width = std::max(pen.x / (float)(1<<16), width);

      pen.x = 0;
      pen.y += face->size->metrics.height << 10;

      str++;
      continue;
    }

#if defined(ENABLE_KERNING)
    if(str != begin && FT_HAS_KERNING(face)) {
      FT_Vector kern = { 0, 0 };
      const auto& a = getGlyphRenderData(*(str - 1));

      FT_Get_Kerning(face, a.idx, g.idx, FT_KERNING_DEFAULT, &kern);

      x += kern.x / (float)(1<<6);
      y += kern.y / (float)(1<<6);
    }
#endif

    unsigned base = num_chars*NumCharVerts;

    *pos++ = make_pos(x, y);
    *pos++ = make_pos(x, y+g.height);
    *pos++ = make_pos(x+g.width, y);
    *pos++ = make_pos(x+g.width, y+g.height);
    *uv++ = g.uvs[0];
    *uv++ = g.uvs[1];
    *uv++ = g.uvs[2];
    *uv++ = g.uvs[3];

    *inds++ = base+0; *inds++ = base+1;
    *inds++ = base+3; *inds++ = base+2;
    *inds++ = PrimitiveRestartIndex;

    num_chars++;

    pen.x += g.advance.x;
    pen.y += g.advance.y;

    str++;
  }

  auto s = std::shared_ptr<pString>(new pString, [](auto p) {
    delete p;
  });

  s->m = face;

  s->base = s->offset = ~0u;
  s->num = num_chars*NumCharIndices;

  s->width = std::max(pen.x / (float)(1<<16), width);
  s->height = (pen.y / (float)(1<<16)) + height();

  return s;
}

void Font::draw(const String& str, vec2 pos, vec4 color) const
{
  pString *p_str = str.get();
  assert(p_str->m == *m && "Drawing string with wrong Font!");

  mat4 mvp = xform::ortho(0.0f, 0.0f, 720.0f, 1280.0f, 0.0f, 1.0f)
    *xform::translate(pos.x, pos.y, 0.0f);

  gx::ScopedPipeline sp(pipeline);

  bindFontAltas();
  font_program->use()
    .uniformMatrix4x4(U::font.uModelViewProjection, mvp)
    .uniformInt(U::font.uAtlas, TexImageUnit)
    .uniformVector4(U::font.uColor, color)
    .drawBaseVertex(gx::TriangleFan, p->vtx, p->ind, p_str->base, p_str->offset, p_str->num);
}

void Font::draw(const char *str, vec2 pos, vec4 color) const
{
  draw(string(str), pos, color);
}

void Font::draw(const String& str, vec2 pos, vec3 color) const
{
  draw(str, pos, vec4{ color.r, color.g, color.b, 1.0f });
}

void Font::draw(const char *str, vec2 pos, vec3 color) const
{
  draw(str, pos, vec4{ color.r, color.g, color.b, 1.0f });
}

void Font::draw(const String& str, vec2 pos, Vector4<byte> color) const
{
  vec4 normalized = {
    color.r / 255.0f,
    color.g / 255.0f,
    color.b / 255.0f,
    color.a / 255.0f,
  };
  draw(str, pos, normalized);
}

void Font::draw(const char *str, vec2 pos, Vector4<byte> color) const
{
  draw(string(str), pos, color);
}

float Font::width(const String& str) const
{
  return str->width;
}

float Font::height(const String& str) const
{
  return str->height;
}

unsigned Font::num(const String& str) const
{
  return str->num;
}

float Font::ascender() const
{
  return m->get()->size->metrics.ascender >> 6;
}

float Font::descender() const
{
  return m->get()->size->metrics.descender >> 6;
}

float Font::height() const
{
  return m->get()->size->metrics.height / (float)(1<<6);
}

float Font::bearingY() const
{
  return m_bearing_y;
}

void Font::bindFontAltas() const
{
  gx::tex_unit(TexImageUnit, m_atlas, m_sampler);
}

void Font::populateRenderData(const std::vector<pGlyph>& glyphs)
{
  int total_area = 0;
  for(auto& glyph : glyphs) {
    auto g = (FT_BitmapGlyph)glyph.m;

    auto area = (g->bitmap.width+2) * (g->bitmap.rows+2);
    total_area += area;
  }

  int atlas_a = pow2_round(sqrt(total_area));
  ivec2 atlas_sz = {
    atlas_a,
    atlas_a*(atlas_a/2) >= total_area ? atlas_a/2 : atlas_a
  };

  stbrp_context ctx;
  std::vector<stbrp_node> nodes(atlas_sz.u);

  stbrp_init_target(&ctx, atlas_sz.u, atlas_sz.v, nodes.data(), nodes.size());

  std::vector<stbrp_rect> rects;
  rects.reserve(glyphs.size());
  for(int i = 0; i < glyphs.size(); i++) {
    auto g = (FT_BitmapGlyph)glyphs[i].m;

    // Spacing around glyphs
    auto w = (stbrp_coord)(g->bitmap.width + 2),
      h = (stbrp_coord)(g->bitmap.rows + 2);

    rects.push_back(stbrp_rect{
      i,
      w, h
    });
  }

  stbrp_pack_rects(&ctx, rects.data(), rects.size());

  std::vector<unsigned char> img(atlas_sz.u*atlas_sz.v);

  // Blit glyphs to atlas
  for(auto& r : rects) {
    auto& g = glyphs[r.id];
    auto bm = ((FT_BitmapGlyph)g.m)->bitmap;

    assert(r.was_packed && "not all glyphs packed into atlas!!!");

    for(unsigned y = 0; y < bm.rows; y++) {
      auto dsty = (atlas_sz.v - (r.y+y))-2;

      auto src = bm.buffer + (y*bm.pitch);
      auto dst = img.data() + (dsty*atlas_sz.u) + r.x + 1;

      for(unsigned x = 0; x < bm.width; x++) *dst++ = *src++;
    }
  }

  // Setup atlas texture and sampler
  std::string atlas_label = "FT_altas";

  m_atlas.init(img.data(), 0, atlas_sz.u, atlas_sz.v, gx::Texture2D::r, gx::Texture2D::u8);
  m_atlas.label((atlas_label + std::to_string(atlas_id++)).c_str());

  m_sampler
    .param(gx::Sampler::MinFilter, gx::Sampler::Linear)
    .param(gx::Sampler::MagFilter, gx::Sampler::Linear)
    .param(gx::Sampler::WrapS, gx::Sampler::EdgeClamp)
    .param(gx::Sampler::WrapT, gx::Sampler::EdgeClamp);

  // Populate render data
  for(unsigned i = 0; i < glyphs.size(); i++) {
    const auto& r = rects[i];
    auto g = glyphs[i].m;
    auto bm = (FT_BitmapGlyph)g;

    u16 x0 = r.x+1, y0 = r.y,
      x1 = r.x+r.w-1, y1 = r.y+bm->bitmap.rows+1;

    y0 = atlas_sz.v - y0;
    y1 = atlas_sz.v - y1;

    UV uvs[] = {
      { x0, y0 }, { x0, y1 },
      { x1, y0 }, { x1, y1 },
    };

    GlyphRenderData rd;

    rd.idx     = glyphs[i].m_idx;
    rd.top     = bm->top;
    rd.left    = bm->left;
    rd.width   = bm->bitmap.width;
    rd.height  = bm->bitmap.rows+1;
    rd.advance = ivec2{ g->advance.x, g->advance.y };

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

int Font::glyphIndex(int ch) const
{
  return m_glyph_index[ch];
}

const Font::GlyphRenderData& Font::getGlyphRenderData(int ch) const
{
  return m_render_data[glyphIndex(ch)];
}

unsigned pFt::alloc(unsigned num_chars)
{
  if(free_list.empty()) return allocAtEnd(num_chars);

  for(auto iter = free_list.begin(); iter != free_list.end(); iter++) {
    if(iter->second < num_chars) continue;

    unsigned ret = iter->first,
      leftover = iter->second-num_chars;

    free_list.erase(iter);
    if(leftover > 0) free_list.push_front({ ret+num_chars, leftover });

    return ret;
  }

  return allocAtEnd(num_chars);
}

void pFt::dealloc(unsigned offset, unsigned num_chars) 
{
  if(ptr == offset+num_chars) {
    ptr -= num_chars;
    return;
  }

  free_list.push_front({ offset, num_chars });
  coalesce();
}

unsigned pFt::allocAtEnd(unsigned num_chars)
{
  if(ptr+num_chars >= pFt::NumBufferChars) return pFt::Error;

  unsigned ret = ptr;
  ptr += num_chars;

  return ret;
}

void pFt::coalesce()
{
  auto in_range = [](std::pair<int, int> a, std::pair<int, int> b) -> bool
  {
    if(a.first < b.first) return a.first+a.second >= b.first;
    else                  return b.first+b.second >= a.first;

    return false;
  };

  auto prev = free_list.begin(),
    iter = ++free_list.begin();
  while(iter != free_list.end()) {
    if(in_range(*prev, *iter)) {
      std::pair<int, int> a = { prev->first, prev->first+prev->second },
        b = { iter->first, iter->first+iter->second };

      iter->first = std::min(a.first, b.first);
      iter->second = std::max(a.second, b.second) - iter->first;

      free_list.erase(prev);
    }
    prev = iter;
    iter++;
  }
}

pFace::pFace(FT_Face f) :
  m(f)
{
}

pFace::~pFace()
{
  if(ft) FT_Done_Face(m);
}

pGlyph::pGlyph(int ch, unsigned idx, FT_Glyph glyph) :
  m_ch(ch), m_idx(idx), m(glyph)
{
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

pString::pString()
{
}

pString::~pString()
{
  if(base != ~0u) p->dealloc(base/4, num);
}

static const std::unordered_map<std::string, std::string> family_to_path = {
  { "arial", "C:\\Windows\\Fonts\\arial.ttf" },
  { "times", "C:\\Windows\\Fonts\\times.ttf" },
  { "georgia", "C:\\Windows\\Fonts\\georgia.ttf" },
  { "calibri", "C:\\Windows\\Fonts\\calibri.ttf" },
  { "consola", "C:\\Windows\\Fonts\\consola.ttf" },
  { "segoeui", "C:\\Windows\\Fonts\\segoeui.ttf" },
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