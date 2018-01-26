#pragma once

#include <common.h>

#include "vmath.h"
#include "texture.h"
#include "vertex.h"
#include "buffer.h"

#include <vector>
#include <string>
#include <memory>

namespace ft {

void init();
void finalize();

using Position = Vector2<i16>;
using UV = Vector2<u16>;

enum {
  TexImageUnit = 15,

  NumCharVerts = 4,
  NumCharIndices = 5,

  PrimitiveRestartIndex = 0xFFFF,
};

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
  String string(const char *str, size_t length) const;
  String string(const std::string& str) const;
  String string(const char *str) const;

  // Returns a string which can only be queried for it's:
  //    width, height
  // and cannot be drawn
  String stringMetrics(const char *str) const;
  String stringMetrics(const std::string& str) const;

  // The returned String can be queried for it's:
  //     width, height, number of generated indices
  //
  // The generated vertices must be drawn as TriangleFans
  //  with a primitive restart index ft::PrimitiveRestartIndex
  //  the return value of:
  //     sampleFontAtlas(samper, uv) 
  //  should be multiplied with the desired font color
  String writeVertsAndIndices(const char *str, StridePtr<Position> pos, StridePtr<UV> uv, u16 *inds) const;

  void draw(const String& str, vec2 pos, vec4 color) const;
  void draw(const char *str, vec2 pos, vec4 color) const;
  void draw(const String& str, vec2 pos, vec3 color) const;
  void draw(const char *str, vec2 pos, vec3 color) const;
  void draw(const String& str, vec2 pos, Vector4<byte> color) const;
  void draw(const char *str, vec2 pos, Vector4<byte> color) const;

  float width(const String& str) const;
  float height(const String& str) const;

  // Number of generated indices
  unsigned num(const String& str) const;

  // Returns NEGATIVE value
  float descender() const;
  float ascender() const;
  float height() const;
  float bearingY() const;

  // Defines sampleFontAtlas
  static const char *frag_shader;

  void bindFontAltas() const;

private:
  struct GlyphRenderData {
    unsigned idx;
    int top, left;
    int width, height;
    ivec2 advance;

    UV uvs[4];
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