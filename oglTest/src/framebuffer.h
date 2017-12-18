#pragma once

#include <GL/glew.h>

#include "math.h"
#include "texture.h"

namespace gx {

class Renderbuffer;

class Framebuffer {
public:
  enum class BindTarget {
    Framebuffer, Read, Draw,
  };

  enum Attachment {
    Color0 = 0x8000,

    DepthStencil = 1, 
  };

  enum BlitMask : unsigned {
    ColorBit = GL_COLOR_BUFFER_BIT,
    DepthBit = GL_DEPTH_BUFFER_BIT,
    StencilBit = GL_STENCIL_BUFFER_BIT,
  };

  Framebuffer();
  Framebuffer(const Framebuffer&) = delete;
  ~Framebuffer();

  void use();
  void use(BindTarget target);

  void tex(const Texture2D& tex, unsigned level, Attachment att);

  void blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);

  static void bind_window(BindTarget target);

private:
  static GLenum binding(BindTarget t);
  static GLenum attachement(Attachment att);

  GLenum m_bound;
  GLuint m;
};

}
