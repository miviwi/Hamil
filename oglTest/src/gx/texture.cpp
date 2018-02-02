#include <gx/texture.h>

#include <cassert>
#include <cstring>
#include <cstdio>

#include <windows.h>

namespace gx {

static void setDefaultParameters(GLenum target);

Texture2D::Texture2D(Format format) :
  m_format(format), m_samples(0)
{
  glGenTextures(1, &m);
}

Texture2D::~Texture2D()
{
  glDeleteTextures(1, &m);
}

void Texture2D::init(unsigned w, unsigned h)
{
  glBindTexture(GL_TEXTURE_2D, m);
  glTexImage2D(GL_TEXTURE_2D, 0, m_format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  setDefaultParameters(GL_TEXTURE_2D);
}

void Texture2D::initMultisample(unsigned samples, unsigned w, unsigned h)
{
  m_samples = samples;

  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, m_format, w, h, GL_TRUE);
}

void Texture2D::init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type type)
{
  assert(format < r8 && "invalid format!");

  glBindTexture(GL_TEXTURE_2D, m);
  glTexImage2D(GL_TEXTURE_2D, mip, m_format, w, h, 0, format, type, data);

  setDefaultParameters(GL_TEXTURE_2D);
}

void Texture2D::upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
                       Format format, Type type)
{
  assert(format < r8 && "invalid format!");

  glBindTexture(GL_TEXTURE_2D, m);
  glTexSubImage2D(GL_TEXTURE_2D, mip, x, y, w, h, format, type, data);
}

void Texture2D::label(const char *lbl)
{
#if !defined(NDEBUG)
  glObjectLabel(GL_TEXTURE, m, strlen(lbl), lbl);
#endif
}

Sampler::Sampler() :
  m_ref(new unsigned(1))
{
  glGenSamplers(1, &m);
}

Sampler::Sampler(const Sampler& other) :
  m(other.m), m_ref(other.m_ref)
{
  *m_ref++;
}

Sampler::~Sampler()
{
  *m_ref--;

  if(!*m_ref) {
    glDeleteSamplers(1, &m);
    delete m_ref;
  }
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
    GL_TEXTURE_MAX_ANISOTROPY_EXT,
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

unsigned p_active_texture = 0;

void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler)
{
  glBindSampler(idx, sampler.m);

  if(idx != p_active_texture) {
    glActiveTexture(GL_TEXTURE0+idx);
    p_active_texture = idx;
  }
  glBindTexture(GL_TEXTURE_2D, tex.m);
}

static void setDefaultParameters(GLenum target)
{
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}