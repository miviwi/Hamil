#pragma once

#include <gx/gx.h>
#include <gx/buffer.h>
#include <gx/pipeline.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <utility>
#include <type_traits>

namespace gx {

class Framebuffer;

class Sampler;

class Texture : public Ref {
public:
  enum Flags {
    Default = 0,
    Multisample = 1<<0,
    Compressed  = 1<<1,
  };

  virtual ~Texture();

  /* --------------- Texture1D init methods ---------------- */
  void init(unsigned w);

  void init(const void *data, unsigned mip, unsigned w, Format format, Type type);
  void upload(const void *data, unsigned mip, unsigned x, unsigned w,
    Format format, Type type);

  /* --------------- Texture2D init methods ---------------- */

  void init(unsigned w, unsigned h); // Initializes MipMap level 0
  void init(ivec2 sz);

  void init(const void *data, unsigned mip, unsigned w, unsigned h, Format format, Type type);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type type);

  // Must be used with Texture::Compressed
  void init(const void *data, unsigned mip, unsigned w, unsigned h, size_t data_size);

  /* -------- Texture3D/Texture2DArray init methods -------- */

  void init(unsigned w, unsigned h, unsigned d /* num_layers */); // Initializes MipMap level 0

  void init(const void *data, unsigned mip, unsigned w, unsigned h, unsigned d, Format format, Type type);
  void upload(const void *data, unsigned mip, unsigned x, unsigned y, unsigned z,
              unsigned w, unsigned h, unsigned d, Format format, Type type);

  /* ------------- TextureCubeMap init methods ------------- */

  void init(Face face, unsigned l); // Initializes MipMap level 0
  void initAllFaces(unsigned l);    // Initializes all Faces

  void init(const void *data, unsigned mip, Face face, unsigned l, Format format, Type type);
  void upload(const void *data, unsigned mip, Face face, unsigned x, unsigned y, unsigned w, unsigned h,
              Format format, Type type);

  // Must be used with Texture::Compressed
  void init(const void *data, unsigned mip, Face face, unsigned l, size_t data_size);

  // Swizzles the Texture's components
  //   eg. swizzle(gx::red, gx::red, gx::red, gx::one) -> texture(...) == vec4(texture(...).rrr, 1.0)
  void swizzle(Component r, Component g, Component b, Component a);

  // Fills (and optionally creates) all mipmaps for the Texture
  void generateMipmaps();

  // Only for internal use!
  void use();

  void label(const char *lbl);

protected:
  friend class Framebuffer;
  friend class PixelBuffer;

  Texture(GLenum target, Format format);

  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

  GLenum m_target;
  Format m_format;

private:
  GLenum m;
};

class Texture1D : public Texture {
public:
  Texture1D(Format format);
};

class Texture2D : public Texture {
public:
  Texture2D(Format format, Flags flags = Default);
  virtual ~Texture2D();

  void initMultisample(unsigned samples, unsigned w, unsigned h);
  void initMultisample(unsigned samples, ivec2 sz);

private:
  friend Framebuffer;

  void assertMultisample();

  unsigned m_samples;
};

class Texture2DArray : public Texture {
public:
  Texture2DArray(Format format, Flags flags = Default);
  virtual ~Texture2DArray();

  void initMultisample(unsigned samples, unsigned w, unsigned h, unsigned layers);

private:
  friend Framebuffer;

  void assertMultisample();

  unsigned m_samples;
};

class TextureCubeMap : public Texture {
public:
  TextureCubeMap(Format format, Flags flags = Default);
  virtual ~TextureCubeMap();
};

class TextureBuffer : public Texture {
public:
  TextureBuffer(Format format, TexelBuffer& buf);

  TextureBuffer& buffer(TexelBuffer& buf);

private:
  TexelBuffer m_buf;
};

class Texture3D : public Texture {
public:
  Texture3D(Format format);

private:
  friend Framebuffer;
};

class TextureHandle : public Ref {
public:
  TextureHandle() :
    m(nullptr)
  { }
  ~TextureHandle();

  template <typename Tex, typename... Args>
  static TextureHandle create(Args... args)
  {
    return new Tex(std::forward<Args>(args)...);
  }

  template <typename T>
  T& get()
  {
    static_assert(std::is_base_of_v<Texture, T>, "T must be a Texture!");

    return (T&)get();
  }

  Texture& get();
  Texture& operator()();

  // Needed for ResourcePool::create(const char *label, ...)
  void label(const char *lbl);

protected:
  TextureHandle(Texture *tex);

private:
  Texture *m;
};

// - param() DOES NOT check the validity of it's arguments
//   (it has an assertion for MagFiler && LinearMipmapLinear
//      that is the only exception, however).
//   Tread lightly!
class Sampler : public Ref {
public:
  enum ParamName {
    MinFilter, MagFilter,
    MinLod, MaxLod,
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

  /* Helpers for creating common Sampler configurations */

  static Sampler repeat1d();
  static Sampler repeat1d_linear();
  static Sampler repeat1d_mipmap();

  static Sampler edgeclamp1d();
  static Sampler edgeclamp1d_linear();
  static Sampler edgeclamp1d_mipmap();

  static Sampler borderclamp1d();
  static Sampler borderclamp1d_linear();
  static Sampler borderclamp1d_mipmap();

  static Sampler repeat2d();
  static Sampler repeat2d_linear();
  static Sampler repeat2d_mipmap();

  static Sampler edgeclamp2d();
  static Sampler edgeclamp2d_linear();
  static Sampler edgeclamp2d_mipmap();

  static Sampler borderclamp2d();
  static Sampler borderclamp2d_linear();
  static Sampler borderclamp2d_mipmap();

  Sampler& param(ParamName name, Param p);
  Sampler& param(ParamName name, float value);
  Sampler& param(ParamName name, vec4 value);

  Sampler& compareRef(CompareFunc func);
  Sampler& noCompareRef();

  void label(const char *lbl);

private:
  friend void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

  static GLenum pname(ParamName name);
  static GLenum param(Param p);

  GLuint m;
};

void tex_unit(unsigned idx, const Texture& tex, const Sampler& sampler);

}