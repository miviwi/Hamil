#include <ui/drawable.h>

#include <gx/fence.h>
#include <gx/commandbuffer.h>
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
  m_staging_tainted(false),
  m_staging_id(gx::ResourcePool::Invalid),
  m_staging_view(std::nullopt),
  m_num_pages(InitialPages),
  m_current_page(0), m_rovers(AtlasSize.s, 0),
  m_pool(pool),
  m_sampler_id(gx::ResourcePool::Invalid),
  m_atlas_id(gx::ResourcePool::Invalid)
{
  m_staging_id = createStaging(numAtlasPages());

  m_fence_id = m_pool.create<gx::Fence>();

  m_sampler_id = m_pool.create<gx::Sampler>("sUiDrawable",
    gx::Sampler::edgeclamp2d());

  m_atlas_id = m_pool.createTexture<gx::Texture2DArray>("t2dUiDrawableAtlas",
    gx::srgb_alpha);
  atlas().get()
    .init(AtlasSize.s, AtlasSize.t, numAtlasPages());
}

bool DrawableManager::prepareDraw()
{
  if(!m_staging_tainted) return false;  // No new images were created
                                        //   no need to wait on the fence

  // Need to reupload the atlas
  uploadAtlas();

  auto& fence = m_pool.get<gx::Fence>(m_fence_id);
  fence.sync();

  m_staging_tainted = false;

  return true; // Need to wait on the fence
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
  m_staging_tainted = true;  // Atlas needs reupload before drawing

  return Drawable(image, this);
}

void DrawableManager::finalize(Drawable *d)
{
  delete d->get();
}

gx::ResourcePool::Id DrawableManager::fenceId() const
{
  return m_fence_id;
}

gx::ResourcePool::Id DrawableManager::samplerId() const
{
  return m_sampler_id;
}

gx::ResourcePool::Id DrawableManager::atlasId() const
{
  return m_atlas_id;
}

void DrawableManager::resizeAtlas(unsigned sz)
{
  auto new_staging_id = createStaging(sz);
  auto new_staging = m_pool.getBuffer(new_staging_id);

  unmapStaging();

  auto staging_ref = staging().get();

  // Copy over the old data
  staging_ref.copy(new_staging(), AtlasSize.area()*numAtlasPages() * sizeof(Color));

  // The old buffer is no longer needed
  staging_ref.destroy();

  m_staging_id = new_staging_id;
  m_num_pages = sz;

  // Resize the atlas and repopulate it
  uploadAtlas();
}

unsigned DrawableManager::numAtlasPages()
{
  return m_num_pages;
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
      // We've found space
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
      resizeAtlas(numAtlasPages()+2); // Grab 2 more pages
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
  auto staging_data = stagingData(coords, page);
  for(unsigned y = 0; y < height; y++) {
    auto src = (Color *)data + y*width;
    auto dst = staging_data + y*AtlasSize.s;

    memcpy(dst, src, width * sizeof(Color));
  }
}

Color *DrawableManager::stagingData(uvec4 coords, unsigned page)
{
  size_t off = coords.x + coords.y*AtlasSize.s;

  return (Color *)mapStaging(page) + off;
}

Color *DrawableManager::mapStaging(unsigned page)
{
  if(!m_staging_view) {
    // Not mapped yet
    m_staging_view = staging().get().map(gx::Buffer::Write, gx::Buffer::MapFlushExplicit);
  }

  return m_staging_view->get<Color>() + AtlasSize.area()*page;
}

void DrawableManager::unmapStaging()
{
  if(!m_staging_view) return;

  m_staging_view->unmap();
  m_staging_view = std::nullopt;
}

gx::ResourcePool::Id DrawableManager::createStaging(unsigned num_pages)
{
  auto id = m_pool.createBuffer<gx::PixelBuffer>("bpUiDrawableStaging",
    gx::Buffer::Dynamic, gx::PixelBuffer::Upload);

  m_pool.getBuffer(id).get()
    .init(sizeof(Color), AtlasSize.area()*num_pages);

  return id;
}

gx::TextureHandle DrawableManager::atlas()
{
  return m_pool.getTexture(m_atlas_id);
}

gx::BufferHandle DrawableManager::staging()
{
  return m_pool.getBuffer(m_staging_id);
}

void DrawableManager::uploadAtlas()
{
  unmapStaging();  // Make sure the buffer isn't mapped

  auto& buf = staging().get<gx::PixelBuffer>();

  buf.uploadTexture(atlas().get(), /* level */ 0,
    AtlasSize.s, AtlasSize.t, numAtlasPages(), gx::rgba, gx::u8);
}

}