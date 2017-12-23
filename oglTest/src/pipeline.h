#pragma once

#include "math.h"

#include <GL/glew.h>

namespace gx {

class Pipeline {
public:
  enum DepthFunc {
    Never = GL_NEVER,
    Always = GL_ALWAYS,
    Less = GL_LESS,
    LessEqual = GL_LEQUAL,
    Greater = GL_GREATER,
    GreaterEqual = GL_GEQUAL,
    Equal = GL_EQUAL,
    NotEqual = GL_NOTEQUAL,
  };

  enum FrontFace {
    Clockwise = GL_CW,
    CounterClockwise = GL_CCW,
  };

  enum CullMode {
    Front = GL_FRONT,
    Back = GL_BACK,
    FrontAndBack = GL_FRONT_AND_BACK,
  };

  Pipeline();

  void use() const;

  Pipeline& viewport(int x, int y, int w, int h);
  Pipeline& scissor(int x, int y, int w, int h);
  Pipeline& noScissor();
  Pipeline& alphaBlend();
  Pipeline& depthTest(DepthFunc func);
  Pipeline& cull(FrontFace front, CullMode mode);
  Pipeline& clearColor(vec4 color);
  Pipeline& clearDepth(float depth);
  Pipeline& clearStencil(int stencil);
  Pipeline& clear(vec4 color, float depth);

private:
  enum ConfigType {
    Viewport, Scissor, Blend,
    Depth, Stencil, Cull,
    Clear,

    NumConfigTypes,
  };

  struct ViewportConfig {
    int x, y;
    int width, height;
  } m_viewport;

  struct ScissorConfig {
    bool set;

    int x, y;
    int width, height;
  } m_scissor;

  struct BlendConfig {
    GLenum sfactor, dfactor;
  } m_blend;

  struct DepthConfig {
    GLenum func;
  } m_depth;

  struct CullConfig {
    GLenum front;
    GLenum mode;
  } m_cull;

  struct ClearConfig {
    vec4 color;
    float depth;
    int stencil;
  } m_clear;

  void disable(ConfigType config) const;
  void enable(ConfigType config) const;

  bool m_enabled[NumConfigTypes];

};

}