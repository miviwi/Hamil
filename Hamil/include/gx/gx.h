#pragma once

#include <common.h>

#include <GL/gl3w.h>

#include <array>
#include <string>

// extension(EXT::TextureSRGB) constants
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F

#define GL_UNSIGNED_INT_10F_11F_11F_REV_EXT 0x8C3B

namespace gx {

class GxInfo;

enum Component  {
  Zero = GL_ZERO, One = GL_ONE,
  Red = GL_RED, Green = GL_GREEN, Blue  = GL_BLUE, Alpha = GL_ALPHA,
};

enum Format : uint {
  r = GL_RED, rg = GL_RG, rgb = GL_RGB, rgba = GL_RGBA,
  bgra = GL_BGRA, bgr = GL_BGR,
  srgb = GL_SRGB, srgb_alpha = GL_SRGB_ALPHA,
  depth = GL_DEPTH_COMPONENT, depth_stencil = GL_DEPTH_STENCIL,

  r8    = GL_R8,    r16     = GL_R16,
  rg8   = GL_RG8,   rg16    = GL_RG16,
  rgb8  = GL_RGB8,  rgb10   = GL_RGB10,    rgb16 = GL_RGB16,
  rgba8 = GL_RGBA8, rgb10a2 = GL_RGB10_A2, rgba16 = GL_RGBA16,

  r11g11b10f = GL_R11F_G11F_B10F,

  rgb565 = GL_RGB565, rgb5a1 = GL_RGB5_A1,

  r16f    = GL_R16F,    r32f    = GL_R32F,
  rg16f   = GL_RG16F,   rg32f   = GL_RG32F,
  rgb16f  = GL_RGB16F,  rgb32f  = GL_RGB32F,
  rgba16f = GL_RGBA16F, rgba32f = GL_RGBA32F,

  depth16 = GL_DEPTH_COMPONENT16,
  depth24 = GL_DEPTH_COMPONENT24,
  depth32 = GL_DEPTH_COMPONENT32, depth32f = GL_DEPTH_COMPONENT32F,
  depth24_stencil8 = GL_DEPTH24_STENCIL8,

  dxt1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxt1_rgba = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
  dxt1_srgb = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, dxt1_srgb_alpha = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
  // gx::rgba color storage
  dxt3 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, dxt5 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
  // gx::srgb_alpha color storage
  dxt3_srgb = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, dxt5_srgb = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,

  // gx::r color storage
  rgtc1 = GL_COMPRESSED_RED_RGTC1, rgtc1s = GL_COMPRESSED_SIGNED_RED_RGTC1,
  // gx::rg color storage
  rgtc2 = GL_COMPRESSED_RG_RGTC2, rgtc2s = GL_COMPRESSED_SIGNED_RG_RGTC2,

  // extension(ARB::TextureBPTC) formats

  // gx::rgba color storage
  bptc = GL_COMPRESSED_RGBA_BPTC_UNORM,
  // gx::srgb_alpha color storage
  bptc_srgb_alpha = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
  // Unsigned floating point
  bptc_uf = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
  // Signed floating point
  bptc_sf = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
};

enum Type : uint {
  i8  = GL_BYTE,  u8  = GL_UNSIGNED_BYTE,
  i16 = GL_SHORT, u16 = GL_UNSIGNED_SHORT,
  i32 = GL_INT,   u32 = GL_UNSIGNED_INT,

  f16 = GL_HALF_FLOAT, f32 = GL_FLOAT, f64 = GL_DOUBLE,
  fixed = GL_FIXED,

  u16_565 = GL_UNSIGNED_SHORT_5_6_5, u16_5551 = GL_UNSIGNED_SHORT_5_5_5_1,
  u32_8888 = GL_UNSIGNED_INT_8_8_8_8, u32_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,

  f10_f10_f11r = GL_UNSIGNED_INT_10F_11F_11F_REV_EXT,

  u16_565r = GL_UNSIGNED_SHORT_5_6_5_REV, u16_1555r = GL_UNSIGNED_SHORT_1_5_5_5_REV,
  u32_8888r = GL_UNSIGNED_INT_8_8_8_8_REV, u32_2_10_10_10r = GL_UNSIGNED_INT_2_10_10_10_REV,
};

enum Face {
  PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X, NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
  PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y, NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
  PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z, NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,

  NumFaces = 6,
};

enum CompareFunc {
  Never   = GL_NEVER,   Always       = GL_ALWAYS,
  Less    = GL_LESS,    LessEqual    = GL_LEQUAL,
  Greater = GL_GREATER, GreaterEqual = GL_GEQUAL,
  Equal   = GL_EQUAL,   NotEqual     = GL_NOTEQUAL,
};

// Ordered according to CubeMap FBO layer indices
static constexpr std::array<Face, NumFaces> Faces = {
  PosX, NegX,
  PosY, NegY,
  PosZ, NegZ,
};

enum {
  // Minimum number of available TexImage units
  //   guaranteed by the OpenGL spec
  NumTexUnits = 16,
  // Minimum number of available gx::UniformBuffer
  //   binding points guaranteed by the OpenGL spec
  NumUniformBindings = 36,
  // Minimum number of Multiple Render Target
  //   bindings gauranteed by the OpenGL spec
  NumMRTBindings = 8,
};

// Returns 'true' when 'fmt' is a color format
bool is_color_format(Format fmt);

// Returns 'true' when 'fmt' corresponds to a compressed format
bool is_compressed_format(Format fmt);

// Must be called AFTER creating a win32::Window!
void init();
void finalize();

GxInfo& info();

// Notes on ResourcePool::create<Texture,Buffer>(const char *label, ...) / .label():
//   - Labels should be in a hungarian-notation type format, where:
//       * p  - gx::Program
//       * b  - gx::Buffer
//       * s  - gx::Sampler
//       * t  - gx::Texture
//       * f  - gx::Fence
//       * q  - gx::Query
//       * a  - gx::VertexArray, ia - gx::IndexedVertexArray
//       * fb - gx::Framebuffer
//       * rb - gx::Framebuffer::renderbuffer<Multisample>()
//   - Suffixes must be appended to the above depending on the particular
//     type of object:
//       * Textures:
//           1d  - gx::Texture1D
//           2d  - gx::Texture2D
//           2da - gx::Texture2DArray
//           c   - gx::TextureCubeMap
//           b   - gx::TextureBuffer
//       * Buffers:
//           v - gx::VertexBuffer
//           i - gx::IndexBuffer
//           u - gx::UniformBuffer
//           t - gx::TexelBuffer
//           p - gx::PixelBuffer
//   - gx::Shader labels should be the same as the parent Program's, but
//     with a suffix appended depending on the type of the Shader:
//       * 'VS' - gx::Shader::Vertex
//       * 'GS' - gx::Shader::Geometry
//       * 'FS' - gx::Shader::Fragment
// Examples:
//   gx::Texture2D texture           -> "t2dTexture"
//   gx::IndexBuffer some_index_buf  -> "biSomeIndexBuf"
//   gx::Framebuffer ui              -> "fbUi"
//     gx::Framebuffer::renderbuffer(..., "rbUi")
//   gx::IndexedVertexArray teapot   -> "iaTeapot"
//   gx::Program skybox              -> "pSkybox"
//     gx::Shader skybox_vs -> "pSkyboxVS"
//     gx::Shader skybox_fs -> "pSkyboxFS"
//   gx::Query sun_occlusion         -> "qSunOcclusion"

void p_bind_VertexArray(unsigned array);
unsigned p_unbind_VertexArray();

}