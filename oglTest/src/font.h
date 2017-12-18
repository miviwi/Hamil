#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <vector>

#include "math.h"
#include "texture.h"

namespace ft {

void init();
void finalize();

class pGlyph {
public:
  pGlyph() : m(0) { }
  pGlyph(int ch, FT_GlyphSlot slot);

  pGlyph(pGlyph&& other);
  pGlyph(const pGlyph& other) = delete;

  ~pGlyph();

private:
  friend class Face;

  int m_ch;
  FT_Glyph m;
};

class Face {
public:
  static constexpr unsigned ATLAS_DIMENSIONS = 256;

  Face(const char *fname, unsigned height);

private:
  FT_Face m;

  gx::Texture2D m_atlas;
  std::vector<vec2> m_uv;
};

}