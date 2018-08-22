#include <ui/drawable.h>
#include <ft/font.h>

#include <array>

#include <cstring>

namespace ui {

struct pDrawable {
  Drawable::Type type;

  virtual const Vertex *vertices() const = 0;
  virtual size_t numVertices() const     = 0;

  virtual const u16 *indices() const = 0;
  virtual size_t numIndices() const  = 0;
};

struct pDrawableText : public pDrawable {
  ft::Font::Ptr font;
  ft::String string;

  std::vector<Vertex> verts;
  std::vector<u16> inds;

  pDrawableText(const ft::Font::Ptr& font_, const char *str);
  pDrawableText(const ft::Font::Ptr& font_, const char *str, size_t sz);

  virtual const Vertex *vertices() const { return verts.data(); }
  virtual size_t numVertices()     const { return verts.size(); }

  virtual const u16 *indices() const { return inds.data(); }
  virtual size_t numIndices()  const { return inds.size(); }
};

struct pDrawableImage : public pDrawable {
  uvec4 coords;
  unsigned page;

  std::array<Vertex, 4> verts;
  std::array<u16, 4> inds;

  pDrawableImage(uvec4 coords_, unsigned page_);

  virtual const Vertex *vertices() const { return verts.data(); }
  virtual size_t numVertices()     const { return verts.size(); }

  virtual const u16 *indices() const { return inds.data(); }
  virtual size_t numIndices()  const { return inds.size(); }
};

pDrawableText::pDrawableText(const ft::Font::Ptr& font_, const char *str) :
  pDrawableText(font_, str, strlen(str))
{
}

pDrawableText::pDrawableText(const ft::Font::Ptr& font_, const char *str, size_t sz) :
  font(font_),
  verts(sz * ft::NumCharVerts), inds(sz * ft::NumCharIndices)
{
  StridePtr<ft::Position> pos(&verts.data()->pos, sizeof(Vertex));
  StridePtr<ft::UV> uv(&verts.data()->uv, sizeof(Vertex));

  string = font->writeVertsAndIndices(str, pos, uv, inds.data());

  unsigned num = string.num();

  verts.resize(num / ft::NumCharIndices * ft::NumCharVerts);
  inds.resize(num);
}

pDrawableImage::pDrawableImage(uvec4 coords_, unsigned page_) :
  coords(coords_), page(page_),
  inds({ 0, 1, 2, 3 })
{

}

Drawable::Drawable(pDrawable *p) :
  m(p)
{
}

Drawable::~Drawable()
{
  if(!deref()) delete m;
}

pDrawable *Drawable::get() const
{
  return m;
}

DrawableManager::DrawableManager() :
  m_local_atlas(AtlasSize.x*AtlasSize.y * InitialPages),
  m_atlas(gx::rgba8)
{
  m_atlas.init(AtlasSize.x, AtlasSize.y, InitialPages);
}

unsigned DrawableManager::numAtlasPages()
{
  return (unsigned)m_local_atlas.size() / PageSize;
}

Color *DrawableManager::localAtlasData(uvec4 coords, unsigned page)
{
  size_t off = page*PageSize + (coords.x + coords.y*AtlasSize.x);

  return m_local_atlas.data() + off;
}

void DrawableManager::reuploadAtlas()
{
  m_atlas.init(m_local_atlas.data(), /* level */0,
    AtlasSize.x, AtlasSize.y, numAtlasPages(), gx::rgba, gx::u8);
}

void DrawableManager::uploadAtlas(uvec4 coords, unsigned page)
{
  m_atlas.upload(localAtlasData(coords, page), /* level */0,
    coords.x, coords.y, page, coords.z, coords.w, 1, gx::rgba, gx::u8);
}

}