#pragma once

#include <common.h>

#include <util/smallvector.h>
#include <math/geometry.h>

#include <utility>
#include <vector>
#include <memory>

namespace gx {
enum Format : uint;
enum Type : uint;
}

namespace util {

class DDSImage {
public:
  enum Type : ulong {
    Invalid,
    // Texture1D, Texture2D, Texture2DArray
    Flat,
    // TextureCubemap
    Cubemap,
    // Texture3D
    Volume
  };

  enum Format : ulong {
    Unknown = 0,

    RGB8  = 20,
    ARGB8 = 21, XRGB8 = 22,
    ABGR8 = 32, XBGR8 = 33,

    RGB565   = 23,
    XRGB1555 = 24, ARGB1555 = 25,
    ARGB4    = 26, XRGB4    = 30,
    RGB332   = 27, ARGB8332 = 29,

    A2BGR10 = 31, A2RGB10 = 35,
    GR16    = 34, ABGR16  = 36,

    A8  = 28,
    L8  = 50, L16 = 81,
    AL8 = 51, AL4 = 52,

    DXT1 = 0x31545844l,  // 'DXT1'
    DXT2 = 0x32545844l,  // 'DXT2'
    DXT3 = 0x33545844l,  // 'DXT3'
    DXT4 = 0x34545844l,  // 'DXT4'
    DXT5 = 0x35545844l,  // 'DXT5'

    R16F = 111, GR16F = 112, ABGR16F = 113,
    R32F = 114, GR32F = 115, ABGR32F = 116,

    DX10 = 'D' | ('X'<<8) | ('1'<<16) | ('0' << 24),
    IsDX10 = 1ul << (ulong)(sizeof(ulong)*CHAR_BIT - 1),

    // DXGI formats

    RGBA32F = 2,  RGBA32U = 3,  RGBA32I = 4,
    RGB32F  = 6,  RGB32U  = 7,  RGB32I  = 8,
    RG32F   = 16, RG32U   = 17, RG32I   = 18,

    RGBA16 = 11, RGBA16F = 10, RGBA16U = 12, RGBA16I = 14,

    RGB10A2 = 24, RGB10A2U = 25,

    R11G11B10F = 26,

    RGBA8 = 28, SRGB8_A8 = 29,
    BGRA8 = 87, SBGR8_A8 = 91,

    DX10_R32F = 41, R32U = 42, R32I = 43,
    R16 = 56, DX10_R16F = 54, R16U = 57, R16I = 59,
    R8  = 61, R8U  = 62, R8I  = 64,
    RG8 = 49, RG8U = 50, RG8I = 52,
  };

  enum CubemapFace {
    PosX, NegX, PosY, NegY, PosZ, NegZ,
    NumFaces,
  };

  enum LoadFlags : uint {
    LoadDefault = 0,
    FlipV   = 1<<0,
  };

  struct Image {
    ulong width, height, depth;

    std::unique_ptr<byte[]> data;
    size_t sz;
  };

  using MipChain = util::SmallVector<Image, 64>;

  struct Error { };
  struct InvalidDDSError : public Error { };
  struct InvalidCubemapError : public Error { };

  DDSImage() = default;
  DDSImage(const DDSImage& other) = delete;
  DDSImage(DDSImage&& other);

  DDSImage& operator=(DDSImage&& other);

  DDSImage& load(const void *data, size_t sz, uint flags = LoadDefault);

  Type type() const;
  Format format() const;

  // Returns 'true' when the image is in a 
  //   compressed format
  bool compressed() const;

  // Returns 'true' when the image has more
  //   than one level available
  bool hasMipmaps() const;

  // Returns num_mipmaps-1 (i.e. the max value
  //  for the 'mip' parameter of gx::Texture::init())
  uint maxLevel() const;

  // Returns the dimensionality of the image
  //   - 2 is returned when type() == Cubemap
  uint numDimensions() const;

  // Returns a gx::Format siutable for
  //   the image's data
  gx::Format texInternalFormat() const;
  // Returns a gx::Format which can be passed to
  //   Texture::init()/Texture::upload()
  gx::Format texFormat() const;
  // Returns a gx::Type which can be passed to
  //   Texture::init()/Texture::upload()
  gx::Type texType() const;

  // Returns the number of storage required for
  //   each pixel in bytes
  uint bytesPerPixel() const;

  // Returns the size in bytes of all the 
  //   faces/layers and mipmap levels of
  //   the image combined
  size_t byteSize() const;
  // Returns the size in bytes of a given
  //   level of the image (one level of
  //   one face/layer for Cubemaps/Arrays)
  size_t byteSize(uint level = 0) const;

  // Returns the pitch of the rows in bytes
  //   for compressed formats (or the size
  //   of a row for other formats)
  size_t pitch() const;

  // Returns the width of the lowest mip level (level 0)
  ulong width() const;
  // Returns the height of the lowest mip level (level 0)
  ulong height() const;
  // Returns the depth of the lowest mip level (level 0)
  ulong depth() const;

  // Returns the number of layers for an array texture
  //   - If m_type == Cubemap returns 6
  ulong numLayers() const;

  // Returns an ivec2(width(), height())
  ivec2 size2d() const;
  // Returns an ivec3(width(), height(), depth())
  ivec3 size3d() const;

  const Image& image(uint level = 0) const;
  const Image& image(CubemapFace face, uint level = 0) const;
  const Image& imageLayer(uint layer, uint level = 0) const;

  // Calls 'fn' for each layer/face witth that layer's
  //   MipChain as the argument
  template <typename Fn /* void (*)(const MipChain&) */>
  void eachLayer(Fn fn) const
  {
    for(const auto& chain : m_layers) fn(chain);
  }

  // Calls 'fn' for each layer/face with the Image at 'level'
  //   as the argument
  template <typename Fn /* void (*)(const Image&) */>
  void eachLayerLevel(Fn fn, uint level = 0) const
  {
    for(const auto& chain : m_layers) fn(chain.at(level));
  }

  // After calling this method the DDSImage
  //   releases ownership of the data for
  //   a given level of the image
  //  - The pointer must be freed via delete[]
  //    otherwise a memory leak will occur
  void *releaseData(uint level = 0);
  // See note above
  void *releaseData(CubemapFace face, uint level = 0);

private:
  // Size in bytes of an image of size WxH
  //   with format == m_format
  size_t byteSize(ulong w, ulong h);

  uint bytesPerPixelDX10() const;

  // m_pitch must be filled before calling this method!
  void copyData(void *dst, void *src, ulong num_rows, uint flags);

  // Copies 'src' to 'dst' flipping it vertically
  void copyDataFlipV(void *dst, void *src, ulong num_rows);

  Type m_type    = Invalid;
  ulong m_format = Unknown;
  bool m_compressed = false;
  ulong m_pitch = 0;
  ulong m_mips  = 0;

  ulong m_num_layers;
  std::unique_ptr<MipChain[]> m_layers;
};

}