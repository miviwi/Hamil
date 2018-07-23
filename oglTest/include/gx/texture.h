#pragma once

#include <gx/gx.h>
#include <util/ref.h>
#include <math/geometry.h>

namespace gx {

class Sampler;

class Texture {
public:
  Texture(const Texture& other) = delete;
  virtual ~Texture();

  void use();

  // Initializes MipMap level 0
  void init(unsigned w, unsigned h);
  void init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type type);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type type);

  void init(unsigned w, unsigned h, unsigned d /* num_layers */);
  void init(const void *data, unsigned mip, unsigned w, unsigned h, unsigned d, Format format, Type type);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned z,
              unsigned w, unsigned h, unsigned d, Format format, Type type);

  void swizzle(Component r, Component g, Component b, Component a);

  void generateMipmaps();

  // Can only be called after init[Multisample]()
  void label(const char *lbl);

protected:
  Texture(GLenum target, Format format);

  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);
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
  friend class Framebuffer;

  unsigned m_samples;
};

class Texture2DArray : public Texture {
public:
  Texture2DArray(Format format);
  Texture2DArray(const Texture2DArray& other) = delete;
  virtual ~Texture2DArray();

  void initMultisample(unsigned samples, unsigned w, unsigned h, unsigned layers);

private:
  unsigned m_samples;
};

class Sampler : public Ref {
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
  ~Sampler();

  Sampler& param(ParamName name, Param p);
  Sampler& param(ParamName name, float value);
  Sampler& param(ParamName name, vec4 value);

private:
  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

  static GLenum pname(ParamName name);
  static GLenum param(Param p);

  GLuint m;
};

void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

}