#pragma once

#include <ui/uicommon.h>
#include <ui/painter.h>

#include <util/ref.h>
#include <gx/texture.h>

#include <vector>

namespace ui {

struct pDrawable;
struct pDrawableText;
struct pDrawableImage;

class Drawable : public Ref {
public:
  enum Type {
    Invalid,

    Text,
    Image,
  };

  struct Error { };

  struct TypeError : public Error { };

  Drawable() : Drawable(nullptr, nullptr) { }
  ~Drawable();

  Type type() const;

  const Drawable& appendVertices(std::vector<Vertex>& buf) const;
  const Drawable& appendIndices(std::vector<u16>& buf) const;

  // Returns the number of indices for the draw call
  size_t num() const;

  vec2 size() const;

  unsigned imageAtlasPage() const;

  ft::Font& textFont() const;
  Color textColor() const;

private:
  friend class DrawableManager;

  Drawable(pDrawable *p, DrawableManager *man);

  pDrawable *operator->() const { return get(); }
  pDrawable *get() const;

  pDrawableText *getText() const;
  pDrawableImage *getImage() const;

  DrawableManager *m_man;
  pDrawable *m;
};

class DrawableManager {
public:
  enum {
    InitialPages = 2,

    TexImageUnit = gx::NumTexUnits-2,
  };

  static constexpr uvec2 AtlasSize{ 1024, 1024 };
  static constexpr unsigned PageSize = AtlasSize.x * AtlasSize.y;

  DrawableManager();

  Drawable fromText(const ft::Font::Ptr& font, const std::string& str, Color color);
  Drawable fromImage();

  void finalize(Drawable *d);

  void bindImageAtlas(int unit = TexImageUnit) const;

private:
  // Convert to OpenGL uv space (bottom-left corner origin)
  uvec4 atlasCoords(uvec4 coords) const;

  unsigned numAtlasPages();

  Color *localAtlasData(uvec4 coords, unsigned page);

  void reuploadAtlas();
  void uploadAtlas(uvec4 coords, unsigned page);

  std::vector<Color> m_local_atlas;
  gx::Sampler m_sampler;
  gx::Texture2DArray m_atlas;

};

}