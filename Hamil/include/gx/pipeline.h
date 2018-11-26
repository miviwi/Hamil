#pragma once

#include <gx/gx.h>

#include <math/geometry.h>

namespace gx {

class Pipeline {
public:
  enum ConfigType {
    Viewport, Scissor, Blend,
    Depth, Stencil, Cull,
    Clear, Wireframe,
    PrimitiveRestart,
    Mask,
    Cubemap,

    NumConfigTypes,
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
  Pipeline& blendColor(vec4 color);
  Pipeline& depthTest(CompareFunc func);
  Pipeline& cull(FrontFace front, CullMode mode);
  Pipeline& cull(CullMode mode);
  Pipeline& clearColor(vec4 color);
  Pipeline& clearDepth(float depth);
  Pipeline& clearStencil(int stencil);
  Pipeline& clear(vec4 color, float depth);
  Pipeline& wireframe();
  Pipeline& primitiveRestart(unsigned index);

  Pipeline& writeDepthOnly();
  Pipeline& depthStencilMask(bool depth, uint stencil);
  Pipeline& writeColorOnly();
  Pipeline& colorMask(bool red, bool green, bool blue, bool alpha);

  Pipeline& noScissor();
  Pipeline& noBlend();
  Pipeline& noDepthTest();
  Pipeline& noCull();
  Pipeline& filledPolys();

  Pipeline& currentScissor();

  Pipeline& seamlessCubemap();
  Pipeline& noSeamlessCubemap();

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

    vec4 color;
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

  struct MaskConfig {
    bool red, green, blue, alpha;
    bool depth;
    uint stencil;
  } m_mask;

  struct CubemapConfig {
    bool seamless;
  } m_cubemap;

  void disable(ConfigType config) const;
  void enable(ConfigType config) const;

  bool compare(const ConfigType config) const;

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