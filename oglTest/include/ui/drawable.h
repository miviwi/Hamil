#pragma once

#include <ui/uicommon.h>
#include <ui/painter.h>

#include <util/ref.h>
#include <gx/texture.h>

#include <vector>

namespace ui {

struct pDrawable;

class Drawable : public Ref {
public:
  enum Type {
    Invalid,

    Text,
    Image,
  };

  ~Drawable();

private:
  friend class DrawableManager;

  Drawable(pDrawable *p);

  pDrawable *operator->() const { return get(); }
  pDrawable *get() const;

  pDrawable *m;
};

class DrawableManager {
public:
  enum {
    InitialPages = 2
  };

  static constexpr uvec2 AtlasSize{ 1024, 1024 };
  static constexpr unsigned PageSize = AtlasSize.x * AtlasSize.y;

  DrawableManager();

private:
  unsigned numAtlasPages();

  Color *localAtlasData(uvec4 coords, unsigned page);

  void reuploadAtlas();
  void uploadAtlas(uvec4 coords, unsigned page);

  std::vector<Color> m_local_atlas;
  gx::Texture2DArray m_atlas;

};

}