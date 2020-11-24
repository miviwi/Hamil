#include <util/dds.h>

#include <math/util.h>
#include <gx/gx.h>

#include <cassert>
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
  FourCC      = 0x00000004l,    // Set when DDSPixelFormat::fourcc has a valid value
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

enum DX10MiscFlags {
  DX10_Cubemap = 0x00000004u,
};

enum DX10ResDimensions {
  Texture1D = 2,
  Texture2D = 3,
  Texture3D = 4,
};

#pragma pack(push, 1)
struct DDSPixelFormat {
  u32 size;    // Size of the structure
  u32 flags;   // Bitwise OR of PixelFormatFlags
  u32 fourcc; // DDSImage::Format
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

struct DDSHeaderDX10 {
  u32 dxgi_format;
  u32 res_dimensions;
  u32 misc_flag;
  u32 array_size;
  u32 reserved_;
};

struct DXT1Block {
  u16 col0, col1;
  u8 row[4];
};
#pragma pack(pop)

// TODO: support integer formats
const std::set<ulong> DDSImage::SupportedFormats = {
    RGB8, ARGB8, XRGB8, ABGR8, XBGR8,
    RGB565, XRGB1555, ARGB1555,
    A2BGR10, A2RGB10, GR16, ABGR16,
    A8, L8, L16, AL8,
    DXT1, DXT2, DXT3, DXT4, DXT5,
    R16F, GR16F, ABGR16F, R32F, GR32F, ABGR32F,

    DX10,

    RGBA32F, /*RGBA32U, RGBA32I,*/
    RGB32F, /*RGB32U, RGB32I,*/
    RG32F, /*RG32U, RG32I,*/
    RGBA16, RGBA16F, /*RGBA16U, RGBA16I,*/
    RGB10A2, /*RGB10A2U,*/
    R11G11B10F,
    RGBA8, SRGB8_A8, BGRA8, SBGR8_A8,
    DX10_R32F, /*R32U, R32I,*/
    R16, DX10_R16F, /*R16U, R16I,*/
    R8, /*R8U, R8I,*/ RG8, /*RG8U, RG8I,*/
    BC1, BC1_srgb, BC2, BC2_srgb, BC3, BC3_srgb,
    BC4, BC4s, BC5, BC5s,
    BC6H_uf, BC6H_sf, BC7, BC7_srgb,
};

DDSImage::DDSImage(DDSImage&& other) :
  m_type(other.m_type),
  m_format(other.m_format),
  m_compressed(other.m_compressed),
  m_pitch(other.m_pitch),
  m_mips(other.m_mips),
  m_num_layers(other.m_num_layers),
  m_layers(std::move(other.m_layers))
{
  other.m_type = Invalid;
  other.m_format = Unknown;
  other.m_compressed = false;
  other.m_pitch = 0;
  other.m_mips = 0;
  other.m_num_layers = 0;
}

DDSImage& DDSImage::operator=(DDSImage&& other)
{
  this->~DDSImage();
  new(this) DDSImage(std::move(other));

  return *this;
}

DDSImage& DDSImage::load(const void *data, size_t data_sz, uint flags)
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
  DDSHeaderDX10 headerdx10;
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

  header.pf.size   = load();
  header.pf.flags  = load();
  header.pf.fourcc = load();
  header.pf.bpp    = load();
  header.pf.r_mask = load(); header.pf.g_mask = load();
  header.pf.b_mask = load(); header.pf.a_mask = load();
  header.caps1 = load();
  header.caps2 = load();

  ptr += sizeof(DDSHeader::reserved2_);

  m_type = Flat;
  if(header.caps2 & Caps2Flags::Cubemap) m_type = Cubemap;
  if((header.caps2 & Caps2Flags::Volume) && (header.depth > 0)) m_type = Volume;

  // Assigned below when format is DXTn/BCPH
  m_compressed = false;

  if(header.pf.flags & PixelFormatFlags::FourCC) {
    m_format = (Format)header.pf.fourcc;

    if(m_format == DX10) {
      // Parse extended header
      headerdx10.dxgi_format    = load();
      headerdx10.res_dimensions = load();
      headerdx10.misc_flag      = load();
      headerdx10.array_size     = load();

      ptr += sizeof(DDSHeaderDX10::reserved_);

      m_format = IsDX10 | headerdx10.dxgi_format;
    }

    switch(m_format & ~IsDX10) {
    case DXT1: case DXT2: case DXT3: case DXT4: case DXT5:

    case BC1: case BC1_srgb: case BC2: case BC2_srgb: case BC3: case BC3_srgb:
    case BC4: case BC4s: case BC5: case BC5s:

    case BC6H_uf: case BC6H_sf:
    case BC7: case BC7_srgb:
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

  checkFormatSupported(); // Can throw UnsupportedFormatError

  m_pitch = header.pitch_or_linear_sz;

  // Fill in the mipmap count if provided
  m_mips = (header.flags & SurfaceDescriptionFlags::MipmapCount) ? header.mipmap_count : 1;

  m_num_layers = 1;  // 1 layer by default
  if(m_format & IsDX10) {   // Use the extended header if it's provided
    m_num_layers = headerdx10.array_size;

    if(headerdx10.misc_flag & DX10MiscFlags::DX10_Cubemap) m_type = Cubemap;
    if(headerdx10.res_dimensions & DX10ResDimensions::Texture3D) m_type = Volume;
  } else if(header.caps2 & Caps2Flags::Cubemap) {
    auto faces = header.caps2 & Caps2Flags::CubemapAll;  // Extract the face flags
    bool has_all_faces = (faces == Caps2Flags::CubemapAll);

    if(!has_all_faces || header.width != header.height) throw InvalidCubemapError();

    m_num_layers = 6;
  }

  m_layers = std::make_unique<MipChain[]>(m_num_layers);
  for(uint n = 0; n < m_num_layers; n++) {
    auto& chain = m_layers[n];
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
    copyData(img, ptr, flags);
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
      copyData(img, ptr, flags);
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

bool DDSImage::sRGB() const
{
  // Non-DXGI formats have no concept of linear/gamma color spaces
  if(!(m_format & ~IsDX10)) return false;

  switch(m_format & ~IsDX10) {
  case SRGB8_A8: case SBGR8_A8:

  case BC1_srgb: case BC2_srgb: case BC3_srgb:
  case BC7_srgb:
    return true;
  }

  return true;
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

gx::Format DDSImage::texInternalFormat() const
{
  if(m_format & IsDX10) return texInternalFormatDX10();

  switch(m_format) {
  case RGB8:
  case XRGB8:
  case XBGR8: return gx::rgb8;

  case ARGB8:
  case ABGR8: return gx::rgba8;

  case RGB565:
  case XRGB1555: return gx::rgb565;

  case ARGB1555: return gx::rgb5a1;

  case A2BGR10:
  case A2RGB10: return gx::rgb10a2;

  case GR16:   return gx::rg16;
  case ABGR16: return gx::rgba16;

  case A8:
  case L8:  return gx::r8;
  case L16: return gx::r16;
  case AL8: return gx::rg8;
  case AL4: return gx::rg;

  // Assume compressed textures are in sRGB color space

  case DXT1: return gx::dxt1_srgb;
  case DXT2: return gx::dxt1_srgb_alpha;
  case DXT3:
  case DXT4: return gx::dxt3_srgb;
  case DXT5: return gx::dxt5_srgb;

  case R16F:    return gx::r16f;
  case GR16F:   return gx::rg16f;
  case ABGR16F: return gx::rgba16f;

  case R32F:    return gx::r32f;
  case GR32F:   return gx::rg32f;
  case ABGR32F: return gx::rgba32f;
  }

  return (gx::Format)~0u;
}

gx::Format DDSImage::texFormat() const
{
  if(m_format & IsDX10) return texFormatDX10();

  switch(m_format) {
  case XBGR8:
  case ABGR8: return gx::rgba;

  case RGB565:
  case RGB8:  return gx::bgr;

  case ARGB8:
  case XRGB8:
  case XRGB1555:
  case ARGB1555:
  case A2RGB10:  return gx::bgra;

  case A2BGR10:
  case ABGR16:  return gx::rgba;

  case GR16: return gx::rg;

  case A8:
  case L8:
  case L16: return gx::r;
  case AL8: return gx::rg;

  case R16F:
  case R32F:    return gx::r;
  case GR16F:
  case GR32F:   return gx::rg;
  case ABGR16F:
  case ABGR32F: return gx::rgba;
  }

  return (gx::Format)~0u;
}

gx::Type DDSImage::texType() const
{
  if(m_format & IsDX10) return texTypeDX10();

  switch(m_format) {
  case RGB8:
  case ARGB8:
  case ABGR8: return gx::Type::u8;

  case XRGB8: return gx::Type::u32_8888r;
  case XBGR8: return gx::Type::u32_8888;

  case RGB565:   return gx::Type::u16_565;
  case XRGB1555:
  case ARGB1555: return gx::Type::u16_5551;

  case A2BGR10:
  case A2RGB10: return gx::Type::u32_10_10_10_2;

  case GR16:
  case ABGR16: return gx::Type::u16;

  case A8:
  case L8:
  case AL8: return gx::Type::u8;
  case L16: return gx::Type::u16;

  case R16F:
  case GR16F:
  case ABGR16F: return gx::Type::f16;

  case R32F:
  case GR32F:
  case ABGR32F: return gx::Type::f32;
  }

  return (gx::Type)~0u;
}

uint DDSImage::bytesPerPixel() const
{
  if(m_format & IsDX10) return bytesPerPixelDX10();

  switch(m_format & ~IsDX10) {
  case RGB8:     return 3;

  case ARGB8:
  case XRGB8:    return 4;

  case RGB565:
  case XRGB1555:
  case ARGB1555:
  case ARGB4:    return 2;

  case RGB332:   return 1;

  case A8:
  case L8:
  case AL4:      return 1;

  case ARGB8332: return 2;
  case XRGB4:    return 2;

  case A2BGR10:
  case ABGR8:
  case XBGR8:
  case GR16:
  case A2RGB10:  return 4;

  case ABGR16:   return 8;

  case AL8:
  case L16:      return 2;

  case DXT1:     return 8;
  case DXT2:
  case DXT3:
  case DXT4:
  case DXT5:     return 16;

  case R16F:     return 2;
  case GR16F:    return 4;
  case ABGR16F:  return 8;
  case R32F:     return 4;
  case GR32F:    return 8;
  case ABGR32F:  return 16;
  }

  return ~0u;
}

size_t DDSImage::byteSize() const
{
  size_t sz = 0;
  for(uint level = 0; level < m_mips; level++) {
    auto& img = image(level);

    sz += img.width*img.height;
  }

  return sz * m_num_layers * bytesPerPixel();
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

ulong DDSImage::numLayers() const
{
  return m_num_layers;
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
  const auto& chain = m_layers[0];

  return chain.at(level);
}

const DDSImage::Image& DDSImage::image(CubemapFace face, uint level) const
{
  const auto& chain = m_layers[face];

  return chain.at(level);
}

const DDSImage::Image& DDSImage::imageLayer(uint layer, uint level) const
{
  const auto& chain = m_layers[layer];

  return chain.at(level);
}

void *DDSImage::releaseData(uint level)
{
  auto& img = m_layers[0].at(level);

  return img.data.release();
}

void *DDSImage::releaseData(CubemapFace face, uint level)
{
  auto& img = m_layers[face].at(level);

  return img.data.release();
}

size_t DDSImage::byteSize(ulong w, ulong h)
{
  if(!m_compressed) return w*h * bytesPerPixel();

  // Divide by the block size rounding up
  w = (w+3) / 4;
  h = (h+3) / 4;

  return w*h * bytesPerPixel();
}

uint DDSImage::bytesPerPixelDX10() const
{
  switch(m_format & ~IsDX10) {
  case RGBA32F:
  case RGBA32U:
  case RGBA32I: return 16;

  case RGB32F:
  case RGB32U:
  case RGB32I:  return 12;

  case RGBA16:
  case RGBA16F:
  case RGBA16U:
  case RGBA16I: return 8;

  case RG32F:
  case RG32U:
  case RG32I:   return 8;

  case RGB10A2U:
  case R11G11B10F:
  case RGB10A2: return 4;

  case SRGB8_A8:
  case SBGR8_A8:
  case RGBA8:
  case BGRA8:   return 4;

  case DX10_R32F:
  case R32U:
  case R32I: return 4;

  case R16:
  case DX10_R16F:
  case R16U:
  case R16I: return 2;

  case R8:
  case R8U:
  case R8I:  return 1;

  case RG8:
  case RG8U:
  case RG8I: return 2;

  // Bytes per BLOCK follow

  case BC1:
  case BC1_srgb: return 8;

  case BC2:
  case BC2_srgb:
  case BC3:
  case BC3_srgb: return 16;

  case BC4:
  case BC4s: return 8;
  case BC5:
  case BC5s: return 16;

  case BC6H_sf:
  case BC6H_uf: return 16;

  case BC7:
  case BC7_srgb: return 16;
  }

  return ~0u;
}

gx::Format DDSImage::texInternalFormatDX10() const
{
  switch(m_format & ~IsDX10) {
  case RGBA32F:   return gx::rgba32f;
  case RGB32F:    return gx::rgb32f;
  case RG32F:     return gx::rg32f;
  case DX10_R32F: return gx::r32f;

  case RGBA16:  return gx::rgba16;
  case RGBA16F: return gx::rgba16f;

  case RGB10A2:    return gx::rgb10a2;
  case R11G11B10F: return gx::r11g11b10f;

  case RGBA8:     return gx::rgba8;
  case SRGB8_A8:  return gx::srgb_alpha;
  case BGRA8:     return gx::rgba;
  case SBGR8_A8:  return gx::srgb_alpha;

  case R8:  return gx::r8;
  case RG8: return gx::rg8;

  case BC1:      return gx::dxt1;
  case BC1_srgb: return gx::dxt1_srgb;

  case BC2:      return gx::dxt3;
  case BC2_srgb: return gx::dxt3_srgb;
  case BC3:      return gx::dxt5;
  case BC3_srgb: return gx::dxt5_srgb;

  case BC4:  return gx::rgtc1;
  case BC4s: return gx::rgtc1s;
  case BC5:  return gx::rgtc2;
  case BC5s: return gx::rgtc2s;

  case BC6H_sf: return gx::bptc_sf;
  case BC6H_uf: return gx::bptc_uf;

  case BC7: return gx::bptc;
  case BC7_srgb: return gx::bptc_srgb_alpha;
  }

  return (gx::Format)~0u;
}

gx::Format DDSImage::texFormatDX10() const
{
  switch(m_format & ~IsDX10) {
  case RGBA32F:   return gx::rgba;
  case RGB32F:    return gx::rgb;
  case RG32F:     return gx::rg;
  case DX10_R32F: return gx::r;

  case RGBA16:
  case RGBA16F: return gx::rgba;

  case RGB10A2:    return gx::rgba;
  case R11G11B10F: return gx::rgb;

  case RGBA8:
  case SRGB8_A8: return gx::rgba;
  case BGRA8:
  case SBGR8_A8: return gx::bgra;

  case R8:  return gx::r;
  case RG8: return gx::rg;
  }

  return (gx::Format)~0u;
}

gx::Type DDSImage::texTypeDX10() const
{
  switch(m_format & ~IsDX10) {
  case RGBA32F:
  case RGB32F:
  case RG32F:
  case DX10_R32F: return gx::Type::f32;

  case RGBA16:  return gx::Type::u16;
  case RGBA16F: return gx::Type::f16;

  case RGB10A2:    return gx::Type::u32_2_10_10_10r;
  case R11G11B10F: return gx::Type::f10_f10_f11r;

  case RGBA8:
  case SRGB8_A8:
  case BGRA8:
  case SBGR8_A8:
  case R8:
  case RG8: return gx::Type::u8;
  }

  return (gx::Type)~0u;
}

void DDSImage::copyData(Image& img, void *src, uint flags)
{
  if(flags == LoadDefault) {
    memcpy(img.data.get(), src, img.sz);
  } else if(flags & FlipV) {
    copyDataFlipV(img, src);
  }
}

static void flip_dxt1_block(void *src_ptr, void *dst_ptr)
{
  auto src = (DXT1Block *)src_ptr,
    dst = (DXT1Block *)dst_ptr;

  memcpy(dst, src, sizeof(u16)*2); // Copy the colors

  // Flip the rows
  dst->row[0] = src->row[3];
  dst->row[3] = src->row[0];

  dst->row[1] = src->row[2];
  dst->row[2] = src->row[1];
}

const std::map<ulong, std::pair<size_t, DDSImage::FlipBlockFn>> DDSImage::flip_fns = {
  { DXT1,     { sizeof(DXT1Block), flip_dxt1_block } },
  { BC1,      { sizeof(DXT1Block), flip_dxt1_block } },
  { BC1_srgb, { sizeof(DXT1Block), flip_dxt1_block } },
};

// TODO: DXT flipping
void DDSImage::copyDataFlipV(Image& img, void *src)
{
  if(compressed()) {
    auto it = flip_fns.find(m_format & ~IsDX10);
    assert(it != flip_fns.end() && "Invalid format!");

    auto [block_sz, flip_block] = it->second;
    copyDataFlipVCompressed(img, src, block_sz, flip_block);
  } else {
    // Start at the top
    auto dst_row = (byte *)img.data.get();

    // Start at the bottom (last_row == num_rows-1)
    auto src_row = (byte *)src + (img.height-1)*m_pitch;

    for(ulong y = img.height; y > 0; y--) {
      memcpy(dst_row, src_row, m_pitch);

      dst_row += m_pitch;
      src_row -= m_pitch;
    }
  }
}

void DDSImage::copyDataFlipVCompressed(Image& img, void *src, size_t block_sz, FlipBlockFn flip_block)
{
  ulong w = (img.width+3) / 4,
    h = (img.height+3) / 4;

  auto src_ptr = (byte *)src + w*(h-1)*block_sz;

  StridePtr<byte> src_block(src_ptr, block_sz);         // Bottom
  StridePtr<byte> dst_block(img.data.get(), block_sz);  // Top

  // Flip block-wise
  for(ulong y = 0; y < h; y++) {
    auto src = src_block, dst = dst_block;
    for(ulong x = 0; x < w; x++) {
      flip_block(src.get(), dst.get());

      dst++; src++;
    }

    dst_block += w;
    src_block -= w;
  }
}

void DDSImage::checkFormatSupported()
{
  // Check if the format is supported
  auto supported_it = SupportedFormats.find(m_format & ~IsDX10);
  if(supported_it != SupportedFormats.end()) return;

  // Format unsupported...
  throw UnsupportedFormatError();
}

}
