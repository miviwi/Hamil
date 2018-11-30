#include <gx/texture.h>

#include <cassert>
#include <cstring>
#include <cstdio>

namespace gx {

// Even though the code utilizes only glSampler's the texture
//   parameters must be set or the texture will be 'incomplete'
static void setDefaultParameters(GLenum target);

Texture::Texture(GLenum target, Format format) :
  m_target(target), m_format(format)
{
  glGenTextures(1, &m);
}

Texture::~Texture()
{
  if(deref()) return;

  glDeleteTextures(1, &m);
}

void Texture::use()
{
  glBindTexture(m_target, m);
}

void Texture::init(unsigned w, unsigned h)
{
  GLenum format = is_color_format(m_format) ? GL_RGBA : GL_DEPTH_COMPONENT;

  use();
  glTexImage2D(m_target, 0, m_format, w, h, 0, format, GL_UNSIGNED_BYTE, nullptr);

  setDefaultParameters(m_target);
}

void Texture::init(ivec2 sz)
{
  assert((sz.x >= 0 && sz.y >= 0) && "Attempted to init a Texture with negative size!");

  init((unsigned)sz.x, (unsigned)sz.y);
}

void Texture::init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type type)
{
  use();
  glTexImage2D(m_target, mip, m_format, w, h, 0, format, type, data);

  setDefaultParameters(m_target);
}

void Texture::upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
                       Format format, Type type)
{
  use();
  glTexSubImage2D(m_target, mip, x, y, w, h, format, type, data);
}

void Texture::init(unsigned w, unsigned h, unsigned d)
{
  use();
  glTexImage3D(m_target, 0, m_format, w, h, d, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  setDefaultParameters(m_target);
}

void Texture::init(const void *data, unsigned mip, unsigned w, unsigned h, unsigned d,
  Format format, Type type)
{
  use();
  glTexImage3D(m_target, 0, m_format, w, h, d, 0, format, type, data);

  setDefaultParameters(m_target);
}

void Texture::upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned z,
                     unsigned w, unsigned h, unsigned d, Format format, Type type)
{
  use();
  glTexSubImage3D(m_target, mip, x, y, z, w, h, d, format, type, data);
}

void Texture::init(Face face, unsigned l)
{
  use();
  glTexImage2D(face, 0, m_format, l, l, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  setDefaultParameters(m_target);
}

void Texture::initAllFaces(unsigned l)
{
  use();
  for(auto face : Faces) {
    glTexImage2D(face, 0, m_format, l, l, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  setDefaultParameters(m_target);
}

void Texture::init(const void *data, unsigned mip, Face face, unsigned l, Format format, Type type)
{
  use();
  glTexImage2D(face, mip, m_format, l, l, 0, format, type, data);

  setDefaultParameters(m_target);
}

void Texture::upload(const void *data, unsigned mip, Face face, unsigned x, unsigned y,
  unsigned w, unsigned h, Format format, Type type)
{
  use();
  glTexSubImage2D(face, mip, x, y, w, h, format, type, data);
}

void Texture::swizzle(Component r, Component g, Component b, Component a)
{
  int params[] = {
    r, g, b, a,
  };

  use();
  glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, params);
}

void Texture::generateMipmaps()
{
  glGenerateMipmap(m_target);
}

void Texture::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_TEXTURE, m, -1, lbl);
#endif
}

Texture2D::Texture2D(Format format, Flags flags) :
  Texture(flags & Multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, format), m_samples(0)
{
}

Texture2D::~Texture2D()
{
}

void Texture2D::initMultisample(unsigned samples, unsigned w, unsigned h)
{
  assertMultisample();
  m_samples = samples;

  use();
  glTexImage2DMultisample(m_target, m_samples, m_format, w, h, GL_TRUE);
}

void Texture2D::initMultisample(unsigned samples, ivec2 sz)
{
  assert((sz.x >= 0 && sz.y >= 0) && "Attempted to initMultisample a Texture with negative size!");

  initMultisample(samples, (unsigned)sz.x, (unsigned)sz.y);
}

void Texture2D::assertMultisample()
{
  assert(m_target == GL_TEXTURE_2D_MULTISAMPLE &&
    "Using a Texture2D with multisampling without the 'Multisample' flag!");
}

Texture2DArray::Texture2DArray(Format format, Flags flags) :
  Texture(flags & Multisample ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY, format),
  m_samples(0)
{
}

Texture2DArray::~Texture2DArray()
{
}

void Texture2DArray::initMultisample(unsigned samples, unsigned w, unsigned h, unsigned layers)
{
  assertMultisample();
  m_samples = samples;

  use();
  glTexImage3DMultisample(m_target, m_samples, m_format, w, h, layers, GL_TRUE);
}

void Texture2DArray::assertMultisample()
{
  assert(m_target == GL_TEXTURE_2D_MULTISAMPLE &&
    "Using a Texture2DArray with multisampling without the 'Multisample' flag!");
}

TextureCubeMap::TextureCubeMap(Format format) :
  Texture(GL_TEXTURE_CUBE_MAP, format)
{
}

TextureCubeMap::~TextureCubeMap()
{
}

TextureBuffer::TextureBuffer(Format format, TexelBuffer& buf) :
  Texture(GL_TEXTURE_BUFFER, format),
  m_buf(buf)
{
  buffer(buf);
}

TextureBuffer& TextureBuffer::buffer(TexelBuffer& buf)
{
  m_buf = buf;

  use();
  glTexBuffer(m_target, m_format, m_buf.m);

  return *this;
}

TextureHandle::TextureHandle(Texture *tex) :
  m(tex)
{
}

TextureHandle::~TextureHandle()
{
  if(!m || deref()) return;

  delete m;
}

Texture& TextureHandle::get()
{
  return *m;
}

Texture& TextureHandle::operator()()
{
  return get();
}

void TextureHandle::label(const char *lbl)
{
  get().label(lbl);
}

Sampler::Sampler()
{
  glGenSamplers(1, &m);
}

Sampler::~Sampler()
{
  if(deref()) return;

  glDeleteSamplers(1, &m);
}

Sampler Sampler::repeat2d()
{
  return Sampler()
    .param(WrapS, Repeat)
    .param(WrapT, Repeat)
    .param(MinFilter, Nearest)
    .param(MagFilter, Nearest);
}

Sampler Sampler::repeat2d_linear()
{
  return Sampler()
    .param(WrapS, Repeat)
    .param(WrapT, Repeat)
    .param(MinFilter, Linear)
    .param(MagFilter, Linear);
}

Sampler Sampler::repeat2d_mipmap()
{
  return Sampler()
    .param(WrapS, Repeat)
    .param(WrapT, Repeat)
    .param(MinFilter, LinearMipmapLinear)
    .param(MagFilter, Linear);
}

Sampler Sampler::edgeclamp2d()
{
  return Sampler()
    .param(WrapS, EdgeClamp)
    .param(WrapT, EdgeClamp)
    .param(MinFilter, Nearest)
    .param(MagFilter, Nearest);
}

Sampler Sampler::edgeclamp2d_linear()
{
  return Sampler()
    .param(WrapS, EdgeClamp)
    .param(WrapT, EdgeClamp)
    .param(MinFilter, Linear)
    .param(MagFilter, Linear);
}

Sampler Sampler::edgeclamp2d_mipmap()
{
  return Sampler()
    .param(WrapS, EdgeClamp)
    .param(WrapT, EdgeClamp)
    .param(MinFilter, LinearMipmapLinear)
    .param(MagFilter, Linear);
}

Sampler Sampler::borderclamp2d()
{
  return Sampler()
    .param(WrapS, BorderClamp)
    .param(WrapT, BorderClamp)
    .param(MinFilter, Nearest)
    .param(MagFilter, Nearest);
}

Sampler Sampler::borderclamp2d_linear()
{
  return Sampler()
    .param(WrapS, BorderClamp)
    .param(WrapT, BorderClamp)
    .param(MinFilter, LinearMipmapLinear)
    .param(MagFilter, Linear);
}

Sampler Sampler::borderclamp2d_mipmap()
{
  return Sampler()
    .param(WrapS, BorderClamp)
    .param(WrapT, BorderClamp)
    .param(MinFilter, LinearMipmapLinear)
    .param(MagFilter, Linear);
}

Sampler& Sampler::param(ParamName name, Param p)
{
  assert((name == MagFilter ? p != LinearMipmapLinear : true)
    && "invalid MagFilter value!");

  glSamplerParameteri(m, pname(name), param(p));

  return *this;
}

Sampler& Sampler::param(ParamName name, float value)
{
  glSamplerParameterf(m, pname(name), value);

  return *this;
}

Sampler& Sampler::param(ParamName name, vec4 value)
{
  glSamplerParameterfv(m, pname(name), value);

  return *this;
}

Sampler& Sampler::compareRef(CompareFunc func)
{
  glSamplerParameteri(m, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  glSamplerParameteri(m, GL_TEXTURE_COMPARE_FUNC, func);

  return *this;
}

Sampler& Sampler::noCompareRef()
{
  glSamplerParameteri(m, GL_TEXTURE_COMPARE_MODE, GL_NONE);

  return *this;
}

void Sampler::label(const char *lbl)
{
#if !defined(NDEBUG)
  glObjectLabel(GL_SAMPLER, m, -1, lbl);
#endif
}

GLenum Sampler::pname(ParamName name)
{
  static const GLenum table[] = {
    GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
    GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD,
    GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R,
    GL_TEXTURE_MAX_ANISOTROPY,
    GL_TEXTURE_BORDER_COLOR,
  };
  return table[name];
}

GLenum Sampler::param(Param p)
{
  static const GLenum table[] = {
    GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR,

    GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_REPEAT, GL_MIRRORED_REPEAT,
  };
  return table[p];
}

thread_local unsigned p_active_texture = ~0u;

void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler)
{
  if(idx != p_active_texture) {
    glActiveTexture(GL_TEXTURE0+idx);
    p_active_texture = idx;
  }

  glBindSampler(idx, sampler.m);
  glBindTexture(tex.m_target, tex.m);
}

static void setDefaultParameters(GLenum target)
{
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

}