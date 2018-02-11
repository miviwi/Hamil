#pragma once

#include <GL/glew.h>

#include <gx/gx.h>
#include <gx/texture.h>
#include <math/geometry.h>

#include <vector>

namespace gx {

class Renderbuffer;

// Direct mapping from Frag outputs to ColorAttachements
//   - layout(location = 0) == Color(0)
//   - layout(location = 1) == Color(1)
//   - layout(location = 5) == Color(5)
//   - etc... up to Color(7)
class Framebuffer {
public:
  enum class BindTarget {
    Framebuffer = GL_FRAMEBUFFER,
    Read = GL_READ_FRAMEBUFFER, Draw = GL_DRAW_FRAMEBUFFER,
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

  static Attachment Color(int index);

  Framebuffer();
  Framebuffer(const Framebuffer&) = delete;
  ~Framebuffer();

  Framebuffer& use();
  Framebuffer& use(BindTarget target);

  Framebuffer& tex(const Texture2D& tex, unsigned level, Attachment att);
  Framebuffer& renderbuffer(Format fmt, Attachment att);
  Framebuffer& renderbuffer(unsigned w, unsigned h, Format fmt, Attachment att);
  Framebuffer& renderbufferMultisample(unsigned samples, Format fmt, Attachment att);
  Framebuffer& renderbufferMultisample(unsigned samples, unsigned w, unsigned h, Format fmt, Attachment att);

  void blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);

  static void bind_window(BindTarget target);

private:
  enum {
    NumDrawBuffers = 8,
    DrawBuffersNeedSetup = 1<<(sizeof(unsigned)*8 - 1),
  };

  static GLenum attachement(Attachment att);

  GLuint create_rendebuffer();
  void framebufferRenderbuffer(GLuint rb, Attachment att);

  void checkIfBound();

  // Framebuffer must be bound already!
  // Supports only 2D[Multisample] textures!
  ivec2 getColorAttachement0Dimensions();

  void drawBuffer(Attachment att);
  void setupDrawBuffers();

  GLuint m;
  GLenum m_bound;
  unsigned m_samples;
  unsigned m_draw_buffers;
  std::vector<GLuint> m_rb;
};

void clear(unsigned mask);

}
