#pragma once

#include <gx/gx.h>

#include <math/geometry.h>

#include <array>
#include <variant>

namespace gx {

// Forward declaration
class Program;

class Pipeline {
public:
  enum FrontFace {
    Clockwise = GL_CW,
    CounterClockwise = GL_CCW,
  };

  enum CullMode {
    Front = GL_FRONT,
    Back = GL_BACK,
    FrontAndBack = GL_FRONT_AND_BACK,
  };

  enum StateParam : uint32_t {
    CullNone,
    CullFront = (1<<0), CullBack = (1<<1), CullFrontAndBack = CullFront|CullBack,

    FrontFaceCCW = 0, FrontFaceCW = 1,

    PolygonModeFilled = 0, PolygonModeLines = 1, PolygonModePoints = 2,

    CompareFuncNever = 0, CompareFuncAlways = 1,
    CompareFuncEqual = 2, CompareFuncNotEqual = 3,
    CompareFuncLess = 4, CompareFuncLessEqual = 5,
    CompareFuncGreater = 6, CompareFuncGreaterEqual = 7,

    Factor0 = 0, Factor1 = 1, 
    FactorSrcColor = 2, Factor1MinusSrcColor = 3,
    FactorDstColor = 4, Factor1MinusDstColor = 5,
    FactorSrcAlpha = 6, Factor1MinusSrcAlpha = 7,
    FactorDstAlpha = 8, Factor1MinusDstAlpha = 9,
    FactorConstColor = 10, Factor1MinusConstColor = 11,
    FactorConstAlpha = 12, Factor1MinusConstAlpha = 13,
    FactorSrcAlpha_Saturate = 14,
  };

  struct VertexInput {
    GLuint array;
    Type indices_type = Type::Invalid;

    auto with_array(GLuint array_) -> VertexInput&
    {
      array = array_;

      return *this;
    }

    auto with_indexed_array(GLuint array_, Type inds_type) -> VertexInput&
    {
      array = array_;
      indices_type = inds_type;

      return *this;
    }
  };

  struct InputAssembly {
    u16 /* GLPrimitive  */ primitive;

    u16 primitive_restart = false;
    u32 restart_index;

    auto with_primitive(Primitive prim) -> InputAssembly&
    {
      primitive = (u16)prim;

      return *this;
    }

    auto with_restart_index(u32 index) -> InputAssembly&
    {
      primitive_restart = true;
      restart_index = index;

      return *this;
    }
  };

  struct Viewport {
    u16 x, y, w, h;

    Viewport(u16 w_, u16 h_) :
      x(0), y(0), w(w_), h(h_)
    { }
    Viewport(u16 x_, u16 y_, u16 w_, u16 h_) :
      x(x_), y(y_), w(w_), h(h_)
    { }
  };

  struct Scissor {
    u32 scissor = false;
    u16 x, y, w, h;

    auto no_test() -> Scissor&
    {
      scissor = false;

      return *this;
    }

    auto with_test(u16 x_, u16 y_, u16 w_, u16 h_) -> Scissor&
    {
      scissor = true;
      x = x_;
      y = y_;
      w = w_;
      h = h_;

      return *this;
    }
  };

  struct Rasterizer {
    u32 cull_mode : 2;
    u32 front_face : 1;
    u32 polygon_mode : 2;

    auto no_cull_face(u32 poly_mode = PolygonModeFilled) -> Rasterizer&
    {
      cull_mode = CullNone;
      polygon_mode = poly_mode;

      return *this;
    }

    auto with_cull_face(
        u32 cull, u32 front_face_ = FrontFaceCCW,
        u32 poly_mode = PolygonModeFilled
      ) -> Rasterizer&
    {
      cull_mode = cull;
      front_face = front_face_;
      polygon_mode = poly_mode;

      return *this;
    }
  };

  // TODO: stencil test parameters
  struct DepthStencil {
    u32 depth_test : 1;
    u32 depth_func : 3;

    auto no_depth_test() -> DepthStencil&
    {
      depth_test = false;

      return *this;
    }

    auto with_depth_test(u32 func = CompareFuncLess) -> DepthStencil&
    {
      depth_test = false;
      depth_func = func;

      return *this;
    }
  };

  struct Blend {
    u32 blend : 1;
    u32 src_factor : 4;
    u32 dst_factor : 4;

    auto no_blend() -> Blend&
    {
      blend = false;

      return *this;
    }

    auto alpha_blend() -> Blend&
    {
      blend = true;
      src_factor = FactorSrcAlpha;
      dst_factor = Factor1MinusSrcAlpha;

      return *this;
    }
  };

  struct Program {
    Program *p;

    auto use(Program& program) -> Program&
    {
      p = &program;

      return *this;
    }
  };

  struct RawConfigStruct {
    // 4 u32's of space at most allocated to one StateStruct variant
    u8 raw[4 * sizeof(u32)];
  };

  using ConfigStruct = std::variant<
    std::monostate,

    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,
    Program,

    RawConfigStruct
  >;

  static Pipeline current();

  Pipeline();

  const Pipeline& use() const;

  Pipeline& viewport(int x, int y, int w, int h);
  Pipeline& scissor(int x, int y, int w, int h);
  Pipeline& scissor(ivec4 rect);
  Pipeline& alphaBlend();
  Pipeline& premultAlphaBlend();
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

  bool isEnabled(unsigned what);

private:
  using ConfigStructArray = std::array<ConfigStruct, std::variant_size_v<ConfigStruct>-2>;
  //                                                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //              Don't reserve slots for the null-ConfigStruct object and StateStructRaw
  
  // The values MUST be arranged in accordance with the order
  //   of the StateStruct std::variant Types!
  enum class ConfigTypeId {
    None,

    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,
    Program,

    RawConfigStruct,
  };

  void disable(unsigned config) const;
  void enable(unsigned config) const;

  bool compare(const unsigned config) const;

  size_t m_num_state_structs = 0;
  ConfigStructArray m_config;
};

class ScopedPipeline {
public:
  ScopedPipeline(const Pipeline& p);
  ~ScopedPipeline();

private:
  Pipeline m;
};

}
