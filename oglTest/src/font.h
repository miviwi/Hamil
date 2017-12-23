#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <vector>
#include <memory>

#include "math.h"
#include "texture.h"
#include "vertex.h"
#include "buffer.h"

namespace ft {

void init();
void finalize();

class pGlyph;
class pString;

class FontFamily {
public:
  FontFamily(const char *name);

  const char *getPath() const;

private:
  std::string m_path;
};

using String = std::shared_ptr<pString>;

class Font {
public:
  Font(const FontFamily& family, unsigned height);
  ~Font();

  // Must be drawn with the same Font!
  String string(const char *str);

  void draw(const String& str, vec2 pos, vec4 color);
  void draw(const char *str, vec2 pos, vec4 color);
  void draw(const String& str, vec2 pos, vec3 color);
  void draw(const char *str, vec2 pos, vec3 color);

private:
  struct GlyphRenderData {
    unsigned idx;
    int top, left;
    int width, height;
    FT_Vector advance;

    vec2 uvs[4];
  };

  void populateRenderData(const std::vector<pGlyph>& glyphs);

  int glyphIndex(int ch);
  const GlyphRenderData& getGlyphRenderData(int ch);

  FT_Face m;

  gx::Texture2D m_atlas;
  gx::Sampler m_sampler;
  std::vector<GlyphRenderData> m_render_data;
  std::vector<int> m_glyph_index;
};

}