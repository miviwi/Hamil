#include <gx/texture.h>

#include <cassert>
#include <cstring>
#include <cstdio>

#include <windows.h>

namespace gx {

static void setDefaultParameters(GLenum target);

Texture::Texture(GLenum target, Format format) :
  m_target(target), m_format(format)
{
  glGenTextures(1, &m);
}

Texture::~Texture()
{
  glDeleteTextures(1, &m);
}

void Texture::use()
{
  glBindTexture(m_target, m);
}


void Texture::init(unsigned w, unsigned h)
{
  use();
  glTexImage2D(m_target, 0, m_format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  setDefaultParameters(m_target);
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

void Texture::init(const void *data, unsigned mip, unsigned w, unsigned h, unsigned d, Format format, Type type)
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

void Texture::swizzle(Component r, Component g, Component b, Component a)
{
  int params[] = {
    r, g, b, a,
  };

  use();
  glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, params);
}

void Texture::label(const char *lbl)
{
#if !defined(NDEBUG)
  glObjectLabel(GL_TEXTURE, m, -1, lbl);
#endif
}

Texture2D::Texture2D(Format format) :
  Texture(GL_TEXTURE_2D, format), m_samples(0)
{
}

Texture2D::~Texture2D()
{
}

void Texture2D::initMultisample(unsigned samples, unsigned w, unsigned h)
{
  m_samples = samples;
  if(m_target == GL_TEXTURE_2D) m_target = GL_TEXTURE_2D_MULTISAMPLE;

  use();
  glTexImage2DMultisample(m_target, m_samples, m_format, w, h, GL_TRUE);
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

Sampler& Sampler::param(ParamName name, Param p)
{
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

GLenum Sampler::pname(ParamName name)
{
  static const GLenum table[] = {
    GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
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

unsigned p_active_texture = ~0u;

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
}

Texture2DArray::Texture2DArray(Format format) :
  Texture(GL_TEXTURE_2D_ARRAY, format), m_samples(0)
{
}

Texture2DArray::~Texture2DArray()
{
}

void Texture2DArray::initMultisample(unsigned samples, unsigned w, unsigned h, unsigned layers)
{
  m_samples = samples;
  if(m_target == GL_TEXTURE_2D_ARRAY) m_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

  use();
  glTexImage3DMultisample(m_target, m_samples, m_format, w, h, layers, GL_TRUE);
}

}