#pragma once

#include <GL/glew.h>

namespace ogl {

class Sampler;

class Texture {
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
  };

  Texture(Format format);
  Texture(const Texture& other) = delete;
  ~Texture();

  void init(void *data, unsigned mip, unsigned w, unsigned h, Format format, Type t);
  void upload(void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type t);

private:
  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

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
  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

  static GLenum pname(ParamName name);
  static GLenum param(Param p);

  GLuint m;
};

void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

}