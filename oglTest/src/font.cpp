#include "font.h"

#define STBRP_STATIC
#include <stb_rect_pack/stb_rect_pack.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack/stb_rect_pack.h>

#include <cstdio>
#include <string>

namespace ft {

FT_Library ft;

void init()
{
  auto err = FT_Init_FreeType(&ft);

  if(err) throw "FreeType init error!";
}

void finalize()
{
  auto err = FT_Done_FreeType(ft);

  if(err) throw "FreeType finalize error!";
}

Face::Face(const char *fname, unsigned height) :
  m_atlas(gx::Texture2D::r8)
{
  auto err = FT_New_Face(ft, fname, 0, &m);
  if(err) throw "FreeType face creation error " + std::to_string(err) + "!";

  FT_Set_Pixel_Sizes(m, 0, height);

  std::vector<pGlyph> glyphs;

  for(int i = 0x20; i < 0x7F; i++) {
    FT_Load_Char(m, i, FT_LOAD_RENDER);

    glyphs.emplace_back(i, m->glyph);
  }

  stbrp_context ctx;
  std::vector<stbrp_node> nodes(ATLAS_DIMENSIONS);

  stbrp_init_target(&ctx, ATLAS_DIMENSIONS, ATLAS_DIMENSIONS, nodes.data(), nodes.size());

  std::vector<stbrp_rect> rects;
  rects.reserve(glyphs.size());
  for(int i = 0; i < glyphs.size(); i++) {
    auto g = (FT_BitmapGlyph)glyphs[i].m;

    auto w = (stbrp_coord)(2+g->bitmap.width),
      h = (stbrp_coord)(2+g->bitmap.rows);

    rects.push_back(stbrp_rect{
      i,
      w, h
    });
  }

  stbrp_pack_rects(&ctx, rects.data(), rects.size());

  std::vector<unsigned char> img(ATLAS_DIMENSIONS*ATLAS_DIMENSIONS);

  FILE *fp;
  fopen_s(&fp, "packed.raw", "wb");

  // Blit glyphs to atlas
  for(auto& r : rects) {
    auto& g = glyphs[r.id];
    auto bm = ((FT_BitmapGlyph)g.m)->bitmap;

    for(unsigned y = 0; y < bm.rows; y++) {
      auto src = bm.buffer + (y*bm.pitch);
      auto dst = img.data() + ((r.y+y+1)*ATLAS_DIMENSIONS) + r.x + 1;

      for(unsigned x = 0; x < bm.width; x++) *dst++ = *src++;
    }
  }

  m_atlas.init(img.data(), 0, ATLAS_DIMENSIONS, ATLAS_DIMENSIONS, gx::Texture2D::r, gx::Texture2D::u8);

  gx::Sampler s;
  s.param(gx::Sampler::MinFilter, gx::Sampler::Linear);
  s.param(gx::Sampler::MagFilter, gx::Sampler::Linear);

  s.param(gx::Sampler::WrapS, gx::Sampler::EdgeClamp);
  s.param(gx::Sampler::WrapT, gx::Sampler::EdgeClamp);
  gx::tex_unit(1, m_atlas, s);

  fwrite(img.data(), ATLAS_DIMENSIONS, ATLAS_DIMENSIONS, fp);

  fclose(fp);

}

pGlyph::pGlyph(int ch, FT_GlyphSlot slot) :
  m_ch(ch)
{
  auto err = FT_Get_Glyph(slot, &m);

  if(err) throw "FreeType glyph get error!";
}

pGlyph::pGlyph(pGlyph&& other) :
  m_ch(other.m_ch), m(other.m)
{
  other.m_ch = 0;
  other.m = 0;
}

pGlyph::~pGlyph()
{
  if(m) FT_Done_Glyph(m);
}


}