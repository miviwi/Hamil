#pragma once

#include <gx/gx.h>
#include <math/geometry.h>

#include <GL/glew.h>

namespace gx {

class Sampler;

class Texture2D {
public:
  Texture2D(Format format);
  Texture2D(const Texture2D& other) = delete;
  ~Texture2D();

  void init(unsigned w, unsigned h);
  void initMultisample(unsigned samples, unsigned w, unsigned h);
  void init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type t);

  // Can only be called after init[Multisample]()
  void label(const char *lbl);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  friend class Framebuffer;

  Format m_format;
  unsigned m_samples;
  GLuint m;
};

class Sampler {
public:
  enum ParamName {
    MinFilter, MagFilter,
    WrapS, WrapT, WrapR,
    Anisotropy,
    BorderColor,
  };

  enum Param {
    Nearest, Linear, LinearMipmapLinear,
    EdgeClamp, BorderClamp, Repeat, RepeatMirror,
  };

  Sampler();
  Sampler(const Sampler& other);
  ~Sampler();

  Sampler& param(ParamName name, Param p);
  Sampler& param(ParamName name, float value);
  Sampler& param(ParamName name, vec4 value);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  static GLenum pname(ParamName name);
  static GLenum param(Param p);

  GLuint m;
  unsigned *m_ref;
};

void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

}