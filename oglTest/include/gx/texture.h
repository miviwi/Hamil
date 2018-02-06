#pragma once

#include <gx/gx.h>
#include <math/geometry.h>

namespace gx {

class Sampler;

class Texture {
public:
  Texture(const Texture& other) = delete;
  virtual ~Texture();

  void use();

  void init(unsigned w, unsigned h);
  void init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type t);

  void swizzle(Component r, Component g, Component b, Component a);

  // Can only be called after init[Multisample]()
  void label(const char *lbl);

protected:
  Texture(GLenum target, Format format);

  friend class Framebuffer;

  GLenum m_target;
  Format m_format;
  GLenum m;
};

class Texture2D : public Texture {
public:
  Texture2D(Format format);
  Texture2D(const Texture2D& other) = delete;
  virtual ~Texture2D();

  void initMultisample(unsigned samples, unsigned w, unsigned h);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  friend class Framebuffer;

  unsigned m_samples;
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

  Sampler& operator=(const Sampler& other);

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