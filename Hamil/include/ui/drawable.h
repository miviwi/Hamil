#pragma once

#include <ui/uicommon.h>
#include <ui/painter.h>

#include <util/ref.h>
#include <gx/texture.h>
#include <gx/buffer.h>
#include <gx/resourcepool.h>

#include <vector>
#include <utility>
#include <optional>

namespace ui {

class Ui;

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

  // 'font' MUST have it's atlas texture allocated from the Ui's ResourcePool!
  Drawable fromText(const ft::Font::Ptr& font, const std::string& str, Color color);

  // 'color' must be an array of u8 RGBA values (in that order!)
  //   - The image is assumed to be in sRGB color space
  //   - Can throw ImageTooLargeError when the width or height exceede
  //     the size of the internal atlas (width or height > AtlasSize)
  //   - Once allocated, the images stay in the atlas for the lifetime of
  //     the DrawableManager so take care when dynamically creating images!
  Drawable fromImage(const void *color, unsigned width, unsigned height);

  void finalize(Drawable *d);

  gx::ResourcePool::Id samplerId() const;
  gx::ResourcePool::Id atlasId() const;

protected:
  DrawableManager(gx::ResourcePool& pool);

  void prepareDraw();

private:
  friend Ui;

  gx::ResourcePool::Id createStaging(unsigned num_pages);

  gx::TextureHandle atlas();
  gx::BufferHandle staging();
  Color *stagingData(uvec4 coords, unsigned page);

  Color *mapStaging(unsigned page);
  void unmapStaging();

  void resizeAtlas(unsigned sz);
  unsigned numAtlasPages();

  // Allocates a width x height block from the atlas, reuploads it if
  //   necessary (when more pages are needed)
  std::pair<uvec4, unsigned> atlasAlloc(unsigned width, unsigned height);
  // Copy 'data' into 'm_local_atlas'
  //   - uploadAtlas() must be called after this to see the changes
  void blitAtlas(const Color *data, uvec4 coords, unsigned page);

  void uploadAtlas();

  // Staging PixelBuffer data
  bool m_staging_tainted;
  gx::ResourcePool::Id m_staging_id;
  std::optional<gx::BufferView> m_staging_view;

  // Atlas allocation data
  unsigned m_num_pages;
  unsigned m_current_page;
  std::vector<u16> m_rovers;

  gx::ResourcePool& m_pool;

  gx::ResourcePool::Id m_sampler_id;
  gx::ResourcePool::Id m_atlas_id;

};

}