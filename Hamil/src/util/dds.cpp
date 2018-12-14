#include <util/dds.h>

#include <math/util.h>
#include <gx/gx.h>

#include <cstring>
#include <algorithm>

namespace util {

const char p_dds_magic[] = "DDS ";

enum SurfaceDescriptionFlags : ulong {
  Caps        = 0x00000001l,
  Height      = 0x00000002l,
  Width       = 0x00000004l,
  Pitch       = 0x00000008l,
  PixelFormat = 0x00001000l,
  MipmapCount = 0x00020000l,    // Number of mip levels
  LinearSize  = 0x00080000l,
  Depth       = 0x00800000l,
};

enum PixelFormatFlags : ulong {
  AlphaPixels = 0x00000001l,
  FourCC      = 0x00000004l,    // Set when DDSPixelFormat::four_cc has a valid value
  RGB         = 0x00000040l,
  RGBA        = 0x00000041l,
};

enum Caps1Flags :ulong {
  Complex     = 0x00000008l,
  Texture     = 0x00001000l,
  Mipmap      = 0x00400000l,    // Set when the image has mip levels
};

enum Caps2Flags : ulong {
  Cubemap     = 0x00000200l,    // Set when the image is a Cubemap
  CubemapPosX = 0x00000400l,
  CubemapNegX = 0x00000800l,
  CubemapPosY = 0x00001000l,
  CubemapNegY = 0x00002000l,
  CubemapPosZ = 0x00004000l,
  CubemapNegZ = 0x00008000l,
  CubemapAll  = 0x0000FC00l,    // All Cubemap faces OR'ed together
  Volume      = 0x00200000l,    // Set when the image has depth
};

#pragma pack(push, 1)
struct DDSPixelFormat {
  u32 size;    // Size of the structure
  u32 flags;   // Bitwise OR of PixelFormatFlags
  u32 four_cc; // DDSImage::Format
  u32 bpp;     // Bits per pixel
  u32 r_mask, g_mask, b_mask, a_mask;  // Bitmask for the red, green, blue and alpha
                                       //   channels respectively
};

struct DDSHeader {
  u32 size;          // Size of the structure
  u32 flags;         // Bitwise OR of SurfaceDescriptionFlags
  u32 height;        // Height of the finest level of the image
  u32 width;         // Width of the finest level
  u32 pitch_or_linear_sz;   // The row stride in bytes
  u32 depth;         // Depth of the finest level of the image
  u32 mipmap_count;  // Number of mipmap levels (1 when there aren't any)
  u32 reserved1_[11];
  DDSPixelFormat pf;
  u32 caps1;         // Bitwise OR of Caps1Flags
  u32 caps2;         // Bitwise OR of Caps2Flags
  u32 reserved2_[3];
};
#pragma pack(pop)

DDSImage& DDSImage::load(const void *data, size_t data_sz)
{
  auto ptr = (byte *)data;

  // Checks if 'data' still has at least 'sz' bytes of data
  auto check_size = [&](size_t sz) {
    auto start = (byte *)data;
    auto diff = ptr - start;

    if(data_sz - diff < sz) throw InvalidDDSError();
  };

  check_size(4);
  if(memcmp(ptr, p_dds_magic, 4)) throw InvalidDDSError();
  ptr += 4;

  // Make sure we don't overrun the buffer
  check_size(sizeof(DDSHeader));

  DDSHeader header;
  memset(&header, 0, sizeof(header));

  auto load = [&]() -> u32 {
    auto x = *(u32 *)ptr;
    ptr += sizeof(u32);

    return x;
  };

  header.size   = load();
  header.flags  = load();
  header.height = load();
  header.width  = load();
  header.pitch_or_linear_sz = load();
  header.depth = load();
  header.mipmap_count = load();

  ptr += sizeof(DDSHeader::reserved1_);

  header.pf.size    = load();
  header.pf.flags   = load();
  header.pf.four_cc = load();
  header.pf.bpp     = load();
  header.pf.r_mask = load(); header.pf.g_mask = load();
  header.pf.b_mask = load(); header.pf.a_mask = load();
  header.caps1 = load();
  header.caps2 = load();

  ptr += sizeof(DDSHeader::reserved2_);

  m_type = Flat;
  if(header.caps2 & Caps2Flags::Cubemap) m_type = Cubemap;
  if((header.caps2 & Caps2Flags::Volume) && (header.depth > 0)) m_type = Volume;

  // Assigned below when format is DXT
  m_compressed = false;

  if(header.pf.flags & PixelFormatFlags::FourCC) {
    m_format = (Format)header.pf.four_cc;

    if(m_format == DX10) {
      m_format = IsDX10;
    }

    switch(m_format) {
    case DXT1: case DXT2: case DXT3: case DXT4: case DXT5:
      m_compressed = true;
      break;
    }
  } else if(header.pf.flags == PixelFormatFlags::RGBA && header.pf.bpp == 32) {
    auto r = header.pf.r_mask, g = header.pf.g_mask, b = header.pf.b_mask, a = header.pf.a_mask;
    if(r == 0x000000FFu && g == 0x0000FF00u && b == 0x00FF0000u && a == 0xFF000000u) {
      // RGBA8
      m_format = IsDX10 | RGBA8;
    } else if(r == 0x00FF0000u && g == 0x00FF0000u && b == 0x000000FFu && a == 0xFF000000u) {
      m_format = IsDX10 | BGRA8;
    } else if(r == 0x000003FFu && g == 0x000FFC00u && b == 0x3FF00000u && a == 0xC0000000u) {
      m_format = IsDX10 | RGB10A2;
    } else {
      m_format = IsDX10 | BGRA8;
    }
  } else if(header.pf.flags == PixelFormatFlags::RGB && header.pf.bpp == 24) {
    m_format = RGB8;
  } else if(header.pf.bpp == 8) {
    m_format = L8;
  } else {
    throw InvalidDDSError();
  }

  m_pitch = (header.flags & SurfaceDescriptionFlags::Pitch) ? header.pitch_or_linear_sz : 0;

  // Fill in the mipmap count if provided
  m_mips = (header.flags & SurfaceDescriptionFlags::MipmapCount) ? header.mipmap_count : 1;

  uint num_images = 1;
  if(header.caps2 & Caps2Flags::Cubemap) {
    auto faces = header.caps2 & Caps2Flags::CubemapAll;  // Extract the face flags
    bool has_all_faces = (faces == Caps2Flags::CubemapAll);

    if(!has_all_faces || header.width != header.height) throw InvalidCubemapError();
  }

  for(uint n = 0; n < num_images; n++) {
    auto& chain = m_images.at(n);
    auto& img = chain.at(chain.emplace());

    auto w = header.width;
    auto h = header.height;
    auto d = header.depth;
    auto sz = byteSize(header.width, header.height);

    img.width  = w;
    img.height = h;
    img.depth  = d;

    img.data = std::make_unique<byte[]>(sz);
    img.sz   = sz;

    check_size(sz);
    memcpy(img.data.get(), ptr, sz);
    ptr += sz;

    for(uint i = 0; i < (m_mips-1) && (w || h); i++) {
      // Get dimensions for next level
      w = clamp<ulong>(w>>1, 1, ULONG_MAX);
      h = clamp<ulong>(h>>1, 1, ULONG_MAX);
      d = clamp<ulong>(d>>1, 1, ULONG_MAX);

      auto& img = chain.at(chain.emplace());

      auto sz = byteSize(w, h);

      img.width  = w;
      img.height = h;
      img.depth  = d;

      img.data = std::make_unique<byte[]>(sz);
      img.sz = sz;

      check_size(sz);
      memcpy(img.data.get(), ptr, sz);
      ptr += sz;
    }
  }

  return *this;
}

DDSImage::Type DDSImage::type() const
{
  return m_type;
}

DDSImage::Format DDSImage::format() const
{
  return (Format)m_format;
}

bool DDSImage::compressed() const
{
  return m_compressed;
}

bool DDSImage::hasMipmaps() const
{
  return m_mips > 1;
}

uint DDSImage::maxLevel() const
{
  return m_mips-1;
}

uint DDSImage::numDimensions() const
{
  uint has_height = (height() > 1) ? 1 : 0;  // 2D when height() > 1
  uint has_depth  = (depth() > 1)  ? 1 : 0;  // 3D when depth()  > 1

  return 1 /* must be at least 1D */ + has_height + has_depth;
}

// TODO!
gx::Format DDSImage::texInternalFormat() const
{
  return gx::r;
}

// TODO!
gx::Format DDSImage::texFormat() const
{
  return gx::r;
}

// TODO!
gx::Type DDSImage::texType() const
{
  return gx::u8;
}

uint DDSImage::bytesPerPixel() const
{
  if(m_format & IsDX10) return bytesPerPixelDX10();

  switch(m_format) {
  case RGB8: return 3;

  case ARGB8:
  case XRGB8: return 4;

  case RGB565:
  case XRGB1555:
  case ARGB1555:
  case ARGB4:    return 2;

  case RGB332: return 1;

  case A8:
  case L8:
  case AL4: return 1;

  case ARGB8332: return 2;
  case XRGB4:    return 2;

  case A2BGR10:
  case ABGR8:
  case XBGR8:
  case GR16:
  case A2RGB10: return 4;

  case ABGR16: return 8;

  case AL8:
  case L16: return 2;

  case DXT1:      return 8;
  case DXT2:
  case DXT3:
  case DXT4:
  case DXT5:      return 16;

  case R16F:    return 2;
  case GR16F:   return 4;
  case ABGR16F: return 8;
  case R32F:    return 4;
  case GR32F:   return 8;
  case ABGR32F: return 16;
  }

  return ~0u;
}

size_t DDSImage::byteSize() const
{
  size_t sz = 0;
  uint is_cubemap = (m_type == Cubemap) ? 6 : 1;
  for(uint level = 0; level < m_mips; level++) {
    auto& img = image(level);

    sz += img.width*img.height;
  }

  return sz * bytesPerPixel() * is_cubemap;
}

size_t DDSImage::byteSize(uint level) const
{
  auto& img = image(level);

  return img.width*img.height * bytesPerPixel();
}

size_t DDSImage::pitch() const
{
  return m_pitch;
}

ulong DDSImage::width() const
{
  return image().width;
}

ulong DDSImage::height() const
{
  return image().height;
}

ulong DDSImage::depth() const
{
  return image().depth;
}

ivec2 DDSImage::size2d() const
{
  return uvec2(width(), height()).cast<int>();
}

ivec3 DDSImage::size3d() const
{
  return uvec3(width(), height(), depth()).cast<int>();
}

const DDSImage::Image& DDSImage::image(uint level) const
{
  const auto& chain = m_images.front();

  return chain.at(level);
}

const DDSImage::Image& DDSImage::image(CubemapFace face, uint level) const
{
  const auto& chain = m_images.at(face);

  return chain.at(level);
}

void *DDSImage::releaseData(uint level)
{
  auto& img = m_images.front().at(level);

  return img.data.release();
}

void *DDSImage::releaseData(CubemapFace face, uint level)
{
  auto& img = m_images.at(face).at(level);

  return img.data.release();
}

uint DDSImage::bytesPerPixelDX10() const
{
  switch(m_format & ~IsDX10) {
  case RGBA32:
  case RGBA32F:
  case RGBA32U:
  case RGBA32I: return 16;

  case RGB32:
  case RGB32F:  case RGB32U:
  case RGB32I: return 12;

  case RGBA16:
  case RGBA16F:
  case RGBA16U:
  case RGBA16I: return 8;

  case RG32:
  case RG32F:
  case RG32U:
  case RG32I: return 8;

  case RGB10A2:
  case RGB10A2U:
  case RGB10A2I: 
  case R11G11B10F: return 4;
  }

  return ~0u;
}

size_t DDSImage::byteSize(ulong w, ulong h)
{
  if(!m_compressed) return w*h * bytesPerPixel();

  // Divide by the block size rounding up
  w = (w+3) / 4;
  h = (h+3) / 4;

  return w*h * bytesPerPixel();
}

}