#include <ui/drawable.h>
#include <ft/font.h>

#include <cstring>
#include <array>
#include <algorithm>

namespace ui {

struct pDrawable {
  Drawable::Type type;

  pDrawable(Drawable::Type type_) :
    type(type_)
  { }

  virtual vec2 size() const = 0;

  virtual const Vertex *vertices() const = 0;
  virtual size_t numVertices() const     = 0;

  virtual const u16 *indices() const = 0;
  virtual size_t numIndices() const  = 0;
};

struct pDrawableText : public pDrawable {
  ft::Font::Ptr font;
  ft::String string;
  Color color;

  std::vector<Vertex> verts;
  std::vector<u16> inds;

  pDrawableText(const ft::Font::Ptr& font_, const char *str, Color color_);
  pDrawableText(const ft::Font::Ptr& font_, const char *str, size_t sz, Color color_);

  virtual vec2 size() const;

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

  virtual vec2 size() const;

  virtual const Vertex *vertices() const { return verts.data(); }
  virtual size_t numVertices()     const { return verts.size(); }

  virtual const u16 *indices() const { return inds.data(); }
  virtual size_t numIndices()  const { return inds.size(); }
};

pDrawableText::pDrawableText(const ft::Font::Ptr& font_, const char *str, Color color_) :
  pDrawableText(font_, str, strlen(str), color_)
{
}

pDrawableText::pDrawableText(const ft::Font::Ptr& font_, const char *str, size_t sz, Color color_) :
  pDrawable(Drawable::Text),
  font(font_),
  color(color_),
  verts(sz * ft::NumCharVerts), inds(sz * ft::NumCharIndices)
{
  StridePtr<ft::Position> pos(&verts.data()->pos, sizeof(Vertex));
  StridePtr<ft::UV> uv(&verts.data()->uv, sizeof(Vertex));

  string = font->writeVertsAndIndices(str, pos, uv, inds.data());

  unsigned num = string.num();

  verts.resize(num / ft::NumCharIndices * ft::NumCharVerts);
  inds.resize(num);
}

vec2 pDrawableText::size() const
{
  return { string.width(), string.height() };
}

pDrawableImage::pDrawableImage(uvec4 coords_, unsigned page_) :
  pDrawable(Drawable::Image),
  coords(coords_), page(page_),
  inds({ 0, 1, 2, 3 })
{
  auto make_vert = [](unsigned x, unsigned y, unsigned u, unsigned v) -> Vertex
  {
    Vertex vtx;
    vtx.pos = vec2{ (float)x, (float)y };
    vtx.uv  = { (u16)u, (u16)v };

    return vtx;
  };

  verts = {
    make_vert(0, 0,               coords.x, coords.y),
    make_vert(0, coords.w,        coords.x, coords.y+coords.w-1),
    make_vert(coords.z, coords.w, coords.x+coords.z-1, coords.y+coords.w-1),
    make_vert(coords.z, 0,        coords.x+coords.z-1, coords.y)
  };
}

vec2 pDrawableImage::size() const
{
  return uvec2(coords.z, coords.w).cast<float>();
}

Drawable::Drawable(pDrawable *p, DrawableManager *man) :
  m_man(man), m(p)
{
}

Drawable::~Drawable()
{
  if(get() && !deref()) m_man->finalize(this);
}

Drawable::Type Drawable::type() const
{
  return get() ? get()->type : Invalid;
}

const Drawable& Drawable::appendVertices(std::vector<Vertex>& buf) const
{
  if(!get()) return *this;

  auto verts = get()->vertices();
  buf.insert(buf.end(), verts, verts+get()->numVertices());

  return *this;
}

const Drawable& Drawable::appendIndices(std::vector<u16>& buf) const
{
  if(!get()) return *this;

  auto inds = get()->indices();
  buf.insert(buf.end(), inds, inds+get()->numIndices());

  return *this;
}

const Drawable& Drawable::appendVertices(Vertex **buf, size_t sz) const
{
  if(!get()) return *this;

  auto verts = get()->vertices();
  auto num   = get()->numVertices();

  // Clamp 'num' to the buffer's size
  num = std::min(num, sz);

  memcpy(*buf, verts, num * sizeof(Vertex));
  *buf += num;

  return *this;
}

const Drawable& Drawable::appendIndices(u16 **buf, size_t sz) const
{
  if(!get()) return *this;

  auto inds = get()->indices();
  auto num  = get()->numIndices();

  // Clamp 'num' to the buffer's size
  num = std::min(num, sz);

  memcpy(*buf, inds, num * sizeof(u16));
  *buf += num;

  return *this;
}

size_t Drawable::num() const
{
  return get() ? get()->numIndices() : 0;
}

vec2 Drawable::size() const
{
  return get() ? get()->size() : vec2{ 0, 0 };
}

unsigned Drawable::imageAtlasPage() const
{
  return getImage()->page;
}

ft::Font& Drawable::textFont() const
{
  return *getText()->font.get();
}

Color Drawable::textColor() const
{
  return getText()->color;
}

pDrawable *Drawable::get() const
{
  return m;
}

pDrawableText *Drawable::getText() const
{
  if(type() != Text) throw TypeError();

  return (pDrawableText *)get();
}

pDrawableImage *Drawable::getImage() const
{
  if(type() != Image) throw TypeError();

  return (pDrawableImage *)get();
}

DrawableManager::DrawableManager(gx::ResourcePool& pool) :
  m_local_atlas(AtlasSize.area() * InitialPages),
  m_current_page(0), m_rovers(AtlasSize.s, 0),
  m_pool(pool),
  m_sampler_id(gx::ResourcePool::Invalid),
  m_atlas_id(gx::ResourcePool::Invalid)
{
  m_sampler_id = m_pool.create<gx::Sampler>("sUiDrawable",
    gx::Sampler::edgeclamp2d());

  m_atlas_id = m_pool.createTexture<gx::Texture2DArray>("t2dUiDrawableAtlas",
    gx::srgb_alpha);
  atlas().get()
    .init(AtlasSize.s, AtlasSize.t, InitialPages);
}

Drawable DrawableManager::fromText(const ft::Font::Ptr& font, const std::string& str, Color color)
{
  auto text = new pDrawableText(font, str.data(), str.size(), color);

  return Drawable(text, this);
}

Drawable DrawableManager::fromImage(const void *color, unsigned width, unsigned height)
{
  if(width > AtlasSize.s || height > AtlasSize.t) throw ImageTooLargeError();

  auto block = atlasAlloc(width, height);

  uvec4 coords  = block.first;
  unsigned page = block.second;

  auto image = new pDrawableImage(coords, page);

  blitAtlas((Color *)color, coords, page);
  uploadAtlas(page);

  return Drawable(image, this);
}

void DrawableManager::finalize(Drawable *d)
{
  delete d->get();
}

gx::ResourcePool::Id DrawableManager::samplerId() const
{
  return m_sampler_id;
}

gx::ResourcePool::Id DrawableManager::atlasId() const
{
  return m_atlas_id;
}

unsigned DrawableManager::numAtlasPages()
{
  return (unsigned)m_local_atlas.size() / PageSize;
}

// Taken from the Quake 2 source code - LM_AllocBlock():
//   https://github.com/id-Software/Quake-2/blob/master/ref_gl/gl_rsurf.c
std::pair<uvec4, unsigned> DrawableManager::atlasAlloc(unsigned width, unsigned height)
{
  uvec4 coords = { 0, 0, width, height };

  unsigned best_y = AtlasSize.t;
  unsigned tentative_y = 0;

  for(unsigned i = 0; i < AtlasSize.s - width; i++) {
    tentative_y = 0;

    unsigned j;
    for(j = 0; j < width; j++) {
      auto rover = m_rovers[i+j];
      if(rover >= best_y) break;

      if(rover > tentative_y) tentative_y = rover;
    }

    if(j == width) {
      coords.x = i;
      coords.y = best_y = tentative_y;
    }
  }

  if(best_y+height > AtlasSize.t) {
    // Not enough space in the current page
    std::fill(m_rovers.begin(), m_rovers.end(), 0);
    m_current_page++;  // Go to the next page

    if(m_current_page >= numAtlasPages()) {
      // We've exhausted all the pages
      m_local_atlas.resize(AtlasSize.area() * (m_current_page+1));

      // Resize the texture
      reuploadAtlas();
    }

    // Retry
    return atlasAlloc(width, height);
  }

  // Move the rovers forward
  for(unsigned i = 0; i < width; i++) {
    m_rovers[coords.x + i] = best_y + height;
  }

  return std::make_pair(coords, m_current_page);
}

void DrawableManager::blitAtlas(const Color *data, uvec4 coords, unsigned page)
{
  unsigned width = coords.z, height = coords.w;
  auto local_atlas = localAtlasData(coords, 0);
  for(unsigned y = 0; y < height; y++) {
    auto src = (Color *)data + y*width;
    auto dst = local_atlas + y*AtlasSize.s;

    memcpy(dst, src, width * sizeof(Color));
  }
}

Color *DrawableManager::localAtlasData(uvec4 coords, unsigned page)
{
  size_t off = page*PageSize + (coords.x + coords.y*AtlasSize.s);

  return m_local_atlas.data() + off;
}

gx::TextureHandle DrawableManager::atlas()
{
  return m_pool.getTexture(m_atlas_id);
}

void DrawableManager::reuploadAtlas()
{
  atlas().get()
    .init(m_local_atlas.data(), /* level */0,
      AtlasSize.s, AtlasSize.t, numAtlasPages(), gx::rgba, gx::u8);
}

void DrawableManager::uploadAtlas(unsigned page)
{
  atlas().get()
    .upload(localAtlasData({ 0, 0, 0, 0 }, page), /* level */0,
      0, 0, page, AtlasSize.s, AtlasSize.t, 1, gx::rgba, gx::u8);
}

}