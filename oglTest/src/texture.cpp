#include "texture.h"

#include <cassert>

namespace ogl {

Texture::Texture(Format format) :
  m_format(format)
{
  glGenTextures(1, &m);
}

Texture::~Texture()
{
  glDeleteTextures(1, &m);
}

void Texture::init(void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t)
{
  assert(format < r8 && "invalid format!");

  glBindTexture(GL_TEXTURE_2D, m);
  glTexImage2D(GL_TEXTURE_2D, mip, internalformat(m_format), w, h, 0, internalformat(format), type(t), data);
}

void Texture::upload(void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h, Format format, Type t)
{
  assert(format < r8 && "invalid format!");

  glBindTexture(GL_TEXTURE_2D, m);
  glTexSubImage2D(GL_TEXTURE_2D, mip, x, y, w, h, internalformat(format), type(t), data);
}

GLenum Texture::internalformat(Format format)
{
  static const GLenum table[] = {
    GL_RED, GL_RG, GL_RGB, GL_RGBA,
    GL_DEPTH, GL_DEPTH_STENCIL,

    GL_R8, GL_R16,
    GL_RGB5_A1, GL_RGBA8,
  };
  return table[format];
}

GLenum Texture::type(Type t)
{
  static const GLenum table[] = {
    GL_BYTE, GL_UNSIGNED_BYTE,
    GL_SHORT, GL_UNSIGNED_SHORT,
    GL_INT, GL_UNSIGNED_INT,
    GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_5_5_1,
  };
  return table[t];
}

Sampler::Sampler()
{
  glGenSamplers(1, &m);
}

Sampler::~Sampler()
{
  glDeleteSamplers(1, &m);
}

void Sampler::param(ParamName name, Param p)
{
  glSamplerParameteri(m, pname(name), param(p));
}

GLenum Sampler::pname(ParamName name)
{
  static const GLenum table[] = {
    GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
    GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
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

void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler)
{
  glBindSampler(GL_TEXTURE0+idx, sampler.m);

  glActiveTexture(GL_TEXTURE0+idx);
  glBindTexture(GL_TEXTURE_2D, tex.m);
}

}