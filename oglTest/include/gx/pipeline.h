#pragma once

#include <math/geometry.h>

#include <GL/glew.h>

namespace gx {

class Pipeline {
public:
  enum ConfigType {
    Viewport, Scissor, Blend,
    Depth, Stencil, Cull,
    Clear, Wireframe,
    PrimitiveRestart,

    NumConfigTypes,
  };

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
  Pipeline& scissor(ivec4 rect);
  Pipeline& alphaBlend();
  Pipeline& additiveBlend();
  Pipeline& subtractiveBlend();
  Pipeline& multiplyBlend();
  Pipeline& depthTest(DepthFunc func);
  Pipeline& cull(FrontFace front, CullMode mode);
  Pipeline& cull(CullMode mode);
  Pipeline& clearColor(vec4 color);
  Pipeline& clearDepth(float depth);
  Pipeline& clearStencil(int stencil);
  Pipeline& clear(vec4 color, float depth);
  Pipeline& wireframe();
  Pipeline& primitiveRestart(unsigned index);

  Pipeline& noScissor();
  Pipeline& noBlend();
  Pipeline& noDepthTest();
  Pipeline& noCull();
  Pipeline& filledPolys();

  Pipeline& currentScissor();

  static Pipeline current();

  bool isEnabled(ConfigType what);

private:
  struct ViewportConfig {
    int x, y;
    int width, height;
  } m_viewport;

  struct ScissorConfig {
    bool current;

    int x, y;
    int width, height;
  } m_scissor;

  struct BlendConfig {
    GLenum mode;
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

  struct RestartConfig {
    unsigned index;
  } m_restart;

  void disable(ConfigType config) const;
  void enable(ConfigType config) const;

  bool compare(ConfigType config) const;

  bool m_enabled[NumConfigTypes];
};

class ScopedPipeline {
public:
  ScopedPipeline(const Pipeline& p);
  ~ScopedPipeline();

private:
  Pipeline m;
};

}