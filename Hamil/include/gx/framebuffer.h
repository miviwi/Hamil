#pragma once

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
//
// Checking for completeness (via status()/complete()) must be done manually!
class Framebuffer {
public:
  enum BindTarget {
    Read = GL_READ_FRAMEBUFFER, Draw = GL_DRAW_FRAMEBUFFER,
  };

  enum Attachment {
    Color0 = 0x8000,

    Depth = 1, 
    DepthStencil = 2,
  };

  enum BlitMask : unsigned {
    ColorBit   = GL_COLOR_BUFFER_BIT,
    DepthBit   = GL_DEPTH_BUFFER_BIT,
    StencilBit = GL_STENCIL_BUFFER_BIT,
  };

  enum Status {
    Complete = GL_FRAMEBUFFER_COMPLETE,

    Undefined   = GL_FRAMEBUFFER_UNDEFINED,
    Unsupported = GL_FRAMEBUFFER_UNDEFINED,

    IncompleteAttachement  = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
    MissingAttachement     = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
    IncompleteDrawBuffer   = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
    IncompleteReadBuffer   = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
    IncompleteMultisample  = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
    IncompleteLayerTargets = GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,
  };

  static Attachment Color(int index);

  Framebuffer();
  Framebuffer(const Framebuffer&) = delete;
  ~Framebuffer();

  Framebuffer& use();
  Framebuffer& use(BindTarget target);

  // The width and height are inferred from the Texture
  Framebuffer& tex(const Texture2D& tex, unsigned level, Attachment att);

  // The width and height are inferred from the Color(0) attachemenet
  //   - Errors will occur if it doesn't exist!
  Framebuffer& renderbuffer(Format fmt, Attachment att);
  Framebuffer& renderbuffer(unsigned w, unsigned h, Format fmt, Attachment att);

  // See the note for renderbuffer(fmt, att)...
  Framebuffer& renderbufferMultisample(unsigned samples, Format fmt, Attachment att);
  Framebuffer& renderbufferMultisample(unsigned samples, unsigned w, unsigned h, Format fmt, Attachment att);

  void blit(Framebuffer& fb, ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);
  void blitToWindow(ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);

  // Alias for blit(fb, rect, rect, mask, filter)
  void copy(Framebuffer& fb, ivec4 rect, unsigned mask, Sampler::Param filter);

  Status status();
  bool complete();

  static void bind_window(BindTarget target);

  void label(const char *label);

private:
  enum {
    NumDrawBuffers = 8,
    DrawBuffersNeedSetup = 1<<(sizeof(unsigned)*CHAR_BIT - 1),
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

  void doBlit(GLuint fb, ivec4 src, ivec4 dst, unsigned mask, Sampler::Param filter);

  GLuint m;
  GLenum m_bound;
  unsigned m_samples;
  unsigned m_draw_buffers;
  std::vector<GLuint> m_rb;
};

// FIXME: no idea where to put this...
void clear(unsigned mask);

}