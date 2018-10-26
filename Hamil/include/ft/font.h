#pragma once

#include <common.h>

#include <math/geometry.h>
#include <util/ref.h>
#include <gx/texture.h>
#include <gx/vertex.h>
#include <gx/buffer.h>
#include <gx/resourcepool.h>

#include <vector>
#include <string>
#include <memory>

namespace ft {

// must be called AFTER gx::init()!
void init();
void finalize();

using Position = Vector2<i16>;
using UV = Vector2<u16>;

enum {
  TexImageUnit = gx::NumTexUnits-1, // Use the last tex unit

  NumCharVerts = 4,
  NumCharIndices = 5,

  PrimitiveRestartIndex = 0xFFFF,
};

// PIMPL classes
class pFace;
class pGlyph;
class pString;
// -------------

class FontFamily {
public:
  FontFamily(const char *name);

  const char *getPath() const;

private:
  std::string m_path;
};

struct String : public Ref {
public:
  String() : m(nullptr) { }
  ~String();

  float width() const;
  float height() const;

  // Number of generated indices
  unsigned num() const;

protected:
  String(pString *p) : m(p) { }

private:
  friend class Font;
  
  pString *operator->() const { return get(); }
  pString *get() const;

  pString *m;
};

class Font {
public:
  using Ptr = std::shared_ptr<Font>;

  Font(const FontFamily& family, unsigned height, gx::ResourcePool *pool = nullptr);
  Font(const Font& other) = delete;
  ~Font();

  int glyphIndex(int ch) const;

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
  void draw(const std::string& str, vec2 pos, vec4 color) const;
  void draw(const char *str, vec2 pos, vec4 color) const;
  void draw(const String& str, vec2 pos, vec3 color) const;
  void draw(const std::string& str, vec2 pos, vec3 color) const;
  void draw(const char *str, vec2 pos, vec3 color) const;
  void draw(const String& str, vec2 pos, Vector4<byte> color) const;
  void draw(const std::string& str, vec2 pos, Vector4<byte> color) const;
  void draw(const char *str, vec2 pos, Vector4<byte> color) const;

  // Returns NEGATIVE value
  float descender() const;
  float ascender() const;
  float height() const;
  float bearingY() const;

  float advance(int glyph_index) const;
  float charAdvance(int ch) const;

  // Defines sampleFontAtlas
  static const char *frag_shader;

  // Can be used ONLY if 'pool' wasn't provided during Font creation!
  void bindFontAltas(int unit = TexImageUnit) const;

  gx::ResourcePool::Id atlasResourceId() const;

  bool monospace() const;
  float monospaceWidth() const;

private:
  struct GlyphRenderData {
    unsigned idx;
    int top, left;
    int width, height;
    ivec2 advance;

    UV uvs[4];
  };

  void populateRenderData(const std::vector<pGlyph>& glyphs, gx::TextureHandle atlas);

  int getGlyphIndex(int ch) const;
  const GlyphRenderData& getGlyphRenderData(int ch) const;

  pFace *m;

  float m_bearing_y;

  gx::ResourcePool::Id m_atlas;
  bool m_atlas_private;

  std::vector<GlyphRenderData> m_render_data;
  std::vector<int> m_glyph_index;
};

}