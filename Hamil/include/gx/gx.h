#pragma once

#include <common.h>

#include <GL/gl3w.h>

#include <array>

namespace gx {

class GxInfo;

enum Component {
  Zero = GL_ZERO, One = GL_ONE,
  Red = GL_RED, Green = GL_GREEN, Blue  = GL_BLUE, Alpha = GL_ALPHA,
};

enum Format {
  r = GL_RED, rg = GL_RG, rgb = GL_RGB, rgba = GL_RGBA,
  bgra = GL_BGRA, bgr = GL_BGR,
  srgb = GL_SRGB, srgb_alpha = GL_SRGB_ALPHA,
  depth = GL_DEPTH_COMPONENT, depth_stencil = GL_DEPTH_STENCIL,

  r8 = GL_R8, r16 = GL_R16,
  rg8 = GL_RG8, rg16 = GL_RG16,
  rgb8 = GL_RGB8, rgb10 = GL_RGB10, rgb565 = GL_RGB565,
  rgb5a1 = GL_RGB5_A1, rgb10a2 = GL_RGB10_A2, rgba8 = GL_RGBA8,

  r16f = GL_R16F, r32f = GL_R32F,
  rg16f = GL_RG16F, rg32f = GL_RG32F,
  rgb16f = GL_RGB16F, rgb32f = GL_RGB32F,
  rgba16f = GL_RGBA16F, rgbaf32 = GL_RGBA32F,

  depth16 = GL_DEPTH_COMPONENT16,
  depth24 = GL_DEPTH_COMPONENT24,
  depth32 = GL_DEPTH_COMPONENT32, depth32f = GL_DEPTH_COMPONENT32F,
  depth24_stencil8 = GL_DEPTH24_STENCIL8,
};

enum Type {
  i8  = GL_BYTE,  u8  = GL_UNSIGNED_BYTE,
  i16 = GL_SHORT, u16 = GL_UNSIGNED_SHORT,
  i32 = GL_INT,   u32 = GL_UNSIGNED_INT,

  f16 = GL_HALF_FLOAT, f32 = GL_FLOAT, f64 = GL_DOUBLE,
  fixed = GL_FIXED,

  u16_565 = GL_UNSIGNED_SHORT_5_6_5, u16_5551 = GL_UNSIGNED_SHORT_5_5_5_1,
  u32_8888 = GL_UNSIGNED_INT_8_8_8_8,
};

enum Face {
  PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X, NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
  PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y, NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
  PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z, NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

enum CompareFunc {
  Never = GL_NEVER, Always = GL_ALWAYS,
  Less = GL_LESS, LessEqual = GL_LEQUAL,
  Greater = GL_GREATER, GreaterEqual = GL_GEQUAL,
  Equal = GL_EQUAL, NotEqual = GL_NOTEQUAL,
};

// Ordered according to CubeMap FBO layer indices
static constexpr std::array<Face, 6> Faces = {
  PosX, NegX,
  PosY, NegY,
  PosZ, NegZ,
};

enum {
  NumTexUnits = 16,
  NumUniformBindings = 36,
};

bool is_color_format(Format fmt);

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
//       * a  - gx::VertexArray, ia - gx::IndexedVertexArray
//       * fb - gx::Framebuffer
//       * rb - gx::Framebuffer::renderbuffer<Multisample>()
//   - Suffixes must be appended to the above depending on the particular
//     type of object:
//       * Textures:
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
// Examples:
//   gx::Texture2D texture           -> "t2dTexture"
//   gx::IndexBuffer some_index_buf  -> "biSomeIndexBuf"
//   gx::Framebuffer ui              -> "fbUi"
//   gx::IndexedVertexArray teapot   -> "iaTeapot"

void p_bind_VertexArray(unsigned array);
unsigned p_unbind_VertexArray();

}