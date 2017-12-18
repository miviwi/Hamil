#pragma once

#include <GL/glew.h>

namespace gx {

class Sampler;

class Texture2D {
public:
  enum Format {
    r, rg, rgb, rgba,
    depth, depth_stencil,

    r8, r16,
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
  void init(void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t);
  void upload(void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type t);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  friend class Framebuffer;

  static GLenum internalformat(Format format);
  static GLenum type(Type t);

  Format m_format;
  GLuint m;
};

class Sampler {
public:
  enum ParamName {
    MinFilter, MagFilter,
    WrapS, WrapT,
  };

  enum Param {
    Nearest, Linear, LinearMipmapLinear,
    EdgeClamp, BorderClamp, Repeat, RepeatMirror,
  };

  Sampler();
  Sampler(const Sampler& other) = delete;
  ~Sampler();

  void param(ParamName name, Param p);

private:
  friend void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

  static GLenum pname(ParamName name);
  static GLenum param(Param p);

  GLuint m;
};

void tex_unit(unsigned idx, const Texture2D& tex, const Sampler& sampler);

}