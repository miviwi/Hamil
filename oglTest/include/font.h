#pragma once

#include <common.h>

#include "vmath.h"
#include "texture.h"
#include "vertex.h"
#include "buffer.h"

#include <vector>
#include <memory>

namespace ft {

void init();
void finalize();

class pFace;
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
  using Ptr = std::shared_ptr<Font>;

  Font(const FontFamily& family, unsigned height);
  Font(const Font& other) = delete;
  ~Font();

  // Must be drawn with the same Font!
  String string(const char *str) const;

  void draw(const String& str, vec2 pos, vec4 color) const;
  void draw(const char *str, vec2 pos, vec4 color) const;
  void draw(const String& str, vec2 pos, vec3 color) const;
  void draw(const char *str, vec2 pos, vec3 color) const;
  void draw(const String& str, vec2 pos, Vector4<byte> color) const;
  void draw(const char *str, vec2 pos, Vector4<byte> color) const;

  float width(const String& str) const;
  float height(const String& str) const;

  float ascender() const;
  float descender() const;

  float height() const;

  float bearingY() const;

private:
  using Position = Vector2<i16>;
  using Uv = Vector2<u16>;

  struct GlyphRenderData {
    unsigned idx;
    int top, left;
    int width, height;
    ivec2 advance;

    Uv uvs[4];
  };

  void populateRenderData(const std::vector<pGlyph>& glyphs);

  int glyphIndex(int ch) const;
  const GlyphRenderData& getGlyphRenderData(int ch) const;

  pFace *m;

  float m_bearing_y;

  gx::Texture2D m_atlas;
  gx::Sampler m_sampler;
  std::vector<GlyphRenderData> m_render_data;
  std::vector<int> m_glyph_index;
};

}