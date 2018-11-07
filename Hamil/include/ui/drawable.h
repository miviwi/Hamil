#pragma once

#include <ui/uicommon.h>
#include <ui/painter.h>

#include <util/ref.h>
#include <gx/texture.h>
#include <gx/resourcepool.h>

#include <vector>
#include <utility>

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

  // Increments *buf by the number of appended vertices
  //   - Appends up to 'sz' Vertices
  const Drawable& appendVertices(Vertex **buf, size_t sz) const;

  // Increments *buf by the number of appended vertices
  //   - Appends up to 'sz' indices
  const Drawable& appendIndices(u16 **buf, size_t sz) const;

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

  static constexpr uvec2 AtlasSize   = { 2048, 1024 };
  static constexpr unsigned PageSize = AtlasSize.area();

  struct Error { };

  struct ImageTooLargeError : public Error { };

  DrawableManager(gx::ResourcePool& pool);

  // 'font' MUST have it's atlas texture allocated from the Ui's ResourcePool!
  Drawable fromText(const ft::Font::Ptr& font, const std::string& str, Color color);

  // 'color' must be an array of u8 RGBA values (in that order!)
  //   - Can throw ImageTooLargeError when the width or height exceede
  //     the size of the internal atlas (width or height > AtlasSize)
  //   - Once allocated, the images stay in the atlas for the lifetime of
  //     the DrawableManager so take care when dynamically creating images!
  Drawable fromImage(const void *color, unsigned width, unsigned height);

  void finalize(Drawable *d);

  gx::ResourcePool::Id samplerId() const;
  gx::ResourcePool::Id atlasId() const;

private:
  gx::TextureHandle atlas();
  Color *localAtlasData(uvec4 coords, unsigned page);

  unsigned numAtlasPages();

  // Allocates a width x height block from the atlas, reuploads it if
  //   necessary (more pages are needed)
  std::pair<uvec4, unsigned> atlasAlloc(unsigned width, unsigned height);
  // Copy 'data' into 'm_local_atlas'
  //   - uploadAtlas(page) must be called after this to see the changes
  void blitAtlas(const Color *data, uvec4 coords, unsigned page);

  void reuploadAtlas();
  void uploadAtlas(unsigned page);

  std::vector<Color> m_local_atlas;

  // Atlas allocation data
  unsigned m_current_page = 0;
  std::vector<unsigned> m_rovers;

  gx::ResourcePool& m_pool;

  gx::ResourcePool::Id m_sampler_id;
  gx::ResourcePool::Id m_atlas_id;

};

}