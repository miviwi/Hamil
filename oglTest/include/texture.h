#pragma once

#include "vmath.h"

#include <GL/glew.h>

namespace gx {

class Sampler;

class Texture2D {
public:
  enum Format {
    r, rg, rgb, rgba,
    depth, depth_stencil,

    r8, r16,
    rgb8,
    rgb5a1, rgba8,

    //i = 0x4000, ui = 0x8000,
  };

  enum Type {
    i8, u8, i16, u16, i32, u32,
    u16_565, u16_5551,
    u32_8888, 
  };

  Texture2D(Format format);
  Texture2D(const Texture2D& other) = delete;
  ~Texture2D();

  void init(unsigned w, unsigned h);
  void initMultisample(unsigned samples, unsigned w, unsigned h);
  void init(void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t);
  void upload(void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type t);

  // Can only be called after init[Multisample]()
  void label(const char *lbl);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  friend class Framebuffer;

  static GLenum internalformat(Format format);
  static GLenum type(Type t);

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