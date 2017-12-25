#pragma once

#include <GL/glew.h>

#include "math.h"
#include "texture.h"

#include <vector>

namespace gx {

class Renderbuffer;

class Framebuffer {
public:
  enum class BindTarget {
    Framebuffer, Read, Draw,
  };

  enum Attachment {
    Color0 = 0x8000,

    Depth = 1, 
    DepthStencil = 2,
  };

  enum BlitMask : unsigned {
    ColorBit = GL_COLOR_BUFFER_BIT,
    DepthBit = GL_DEPTH_BUFFER_BIT,
    StencilBit = GL_STENCIL_BUFFER_BIT,
  };

  enum Format {
    rgb5a1 , rgba8,
    depth16, depth24,
    depth24_stencil8,
  };

  Framebuffer();
  Framebuffer(const Framebuffer&) = delete;
  ~Framebuffer();

  Framebuffer& use();
  Framebuffer& use(BindTarget target);

  Framebuffer& tex(const Texture2D& tex, unsigned level, Attachment att);
  Framebuffer& renderbuffer(Format fmt, Attachment att);
  Framebuffer& renderbuffer(unsigned w, unsigned h, Format fmt, Attachment att);

  void blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);

  static void bind_window(BindTarget target);

private:
  static GLenum binding(BindTarget t);
  static GLenum attachement(Attachment att);
  static GLenum internalformat(Format fmt);

  void checkIfBound();

  // Must be bound already!
  ivec2 getColorAttachementDimensions();

  GLenum m_bound;
  GLuint m;
  std::vector<GLuint> m_rb;
};

void clear(int mask);

}
