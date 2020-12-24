#pragma once

#include <gx/gx.h>

#include <math/geometry.h>

#include <type_traits>
#include <array>
#include <variant>
#include <optional>
#include <algorithm>
#include <utility>

namespace gx {

// Forward declarations
class Program;
class ResourcePool;

class Pipeline {
public:
  using ResourceId = u32;   // Must match ResourcePool::Id

  enum StateParam : u32 {
    CullNone = 0,
    CullFront = (1<<0), CullBack = (1<<1), CullFrontAndBack = CullFront|CullBack,

    FrontFaceCCW = 0, FrontFaceCW = 1,

    PolygonModeFilled = 0, PolygonModeLines = 1, PolygonModePoints = 2,

    CompareFuncNever = 0, CompareFuncAlways = 1,
    CompareFuncEqual = 2, CompareFuncNotEqual = 3,
    CompareFuncLess = 4, CompareFuncLessEqual = 5,
    CompareFuncGreater = 6, CompareFuncGreaterEqual = 7,

    BlendFuncAdd = 0, BlendFuncSub = 1,
    BlendFuncMin = 2, BlendFuncMax = 3,

    Factor0 = 0, Factor1 = 1, 
    FactorSrcColor = 2, Factor1MinusSrcColor = 3,
    FactorDstColor = 4, Factor1MinusDstColor = 5,
    FactorSrcAlpha = 6, Factor1MinusSrcAlpha = 7,
    FactorDstAlpha = 8, Factor1MinusDstAlpha = 9,
    FactorConstColor = 10, Factor1MinusConstColor = 11,
    FactorConstAlpha = 12, Factor1MinusConstAlpha = 13,
    FactorSrcAlpha_Saturate = 14,
  };

  struct RawConfigStruct {
    // 4 u32's of space at most allocated to one StateStruct variant
    u8 raw[4 * sizeof(u32)];
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ VertexInput {
    ResourceId array = ~0u;
    Type indices_type = Type::Invalid;

    auto with_array(ResourceId array_) -> VertexInput&
    {
      array = array_;

      return *this;
    }

    auto with_indexed_array(ResourceId array_, Type inds_type) -> VertexInput&
    {
      array = array_;
      indices_type = inds_type;

      return *this;
    }

    bool isIndexed() const { return indices_type != Type::Invalid; }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ InputAssembly {
    Primitive primitive = gx::Primitive::Invalid;

    u32 primitive_restart = false;
    u32 restart_index = 0;

    auto with_primitive(Primitive prim) -> InputAssembly&
    {
      primitive = prim;

      return *this;
    }

    auto with_restart_index(u32 index) -> InputAssembly&
    {
      primitive_restart = true;
      restart_index = index;

      return *this;
    }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ Viewport {
    u16 x = 0, y = 0, w = 0, h = 0;

    Viewport(u16 w_, u16 h_) :
      x(0), y(0), w(w_), h(h_)
    { }
    Viewport(u16 x_, u16 y_, u16 w_, u16 h_) :
      x(x_), y(y_), w(w_), h(h_)
    { }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ Scissor {
    u32 scissor = false;
    i16 x = 0, y = 0, w = 0, h = 0;

    auto no_test() -> Scissor&
    {
      scissor = false;

      return *this;
    }

    auto with_test(i16 x_, i16 y_, i16 w_, i16 h_) -> Scissor&
    {
      scissor = true;
      x = x_;
      y = y_;
      w = w_;
      h = h_;

      return *this;
    }

    auto with_test(ivec4 sc) -> Scissor&
    {
      return with_test(sc.x, sc.y, sc.z, sc.w);
    }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ Rasterizer {
    u32 cull_mode : 2;
    u32 front_face : 1;
    u32 polygon_mode : 2;

    Rasterizer()
    {
      cull_mode = CullNone;
      front_face = FrontFaceCCW;
      polygon_mode = PolygonModeFilled;
    }

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
  struct /*alignas(sizeof(RawConfigStruct))*/ DepthStencil {
    u32 depth_test : 1;
    u32 depth_func : 3;

    DepthStencil()
    {
      depth_test = false;
      depth_func = CompareFuncNever;
    }

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

  struct /*alignas(sizeof(RawConfigStruct))*/ Blend {
    u32 blend : 1;
    u32 func : 2;
    u32 src_factor : 4;
    u32 dst_factor : 4;

    Blend()
    {
      blend = false;
      func = BlendFuncAdd;
      src_factor = Factor1;
      dst_factor = Factor0;
    }

    auto no_blend() -> Blend&
    {
      blend = false;

      return *this;
    }

    auto alpha_blend() -> Blend&
    {
      blend = true;

      func = BlendFuncAdd;
      src_factor = FactorSrcAlpha;
      dst_factor = Factor1MinusSrcAlpha;

      return *this;
    }

    auto premult_alpha_blend() -> Blend&
    {
      blend = true;

      func = BlendFuncAdd;
      src_factor = Factor1;
      dst_factor = Factor1MinusSrcAlpha;

      return *this;
    }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ ClearColor {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

    ClearColor(float r_, float g_, float b_) :
      r(r_), g(g_), b(b_), a(1.0f)
    { }

    ClearColor(float r_, float g_, float b_, float a_) :
      r(r_), g(g_), b(b_), a(a_)
    { }

    ClearColor(const vec4& c) :
      ClearColor(c.r, c.g, c.b, c.a)
    { }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ ClearDepth {
    float z = 1.0f;

    ClearDepth(float z_) : z(z_) { }

    ClearDepth() = default;
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ SeamlessCubemap {
    u32 seamless = false;

    SeamlessCubemap(bool seamless_) : seamless(seamless_) { }
  };

  struct /*alignas(sizeof(RawConfigStruct))*/ Program {
    ResourceId /* gx::Program */ id = ~0u;

    Program(ResourceId id_) : id(id_) { }
  };

  using ConfigStruct = std::variant<
    std::monostate,

    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,
    ClearColor, ClearDepth,
    SeamlessCubemap,
    Program,

    RawConfigStruct
  >;

  static Pipeline current();

  Pipeline(ResourcePool *pool);

  // Add a ConfigStruct to the configuration of this Pipeline
  //  - When the first (and only) argument is a callable which matches
  //    the signature void(Config&) it will be called with the created
  //    Config object
  //  - Otherwise the method's arguments will be passed to the Config
  //    object's constructor
  //  - A given ConfigStruct object kind can be add()'ed to a Pipeline only
  //    ONCE!
  //
  //   Usage:
  //       auto pipeline = gx::Pipeline()
  //           .add<gx::Pipeline::Viewport>(1280, 720)
  //           .add<gx::Pipeline::InputAssembly>([](auto& ia) {
  //             ia.with_primitive(gx::Primitive::TraingleStrip);
  //           });
  //
  template <typename Config, typename FnOrArg0, typename... Args>
  Pipeline& add(FnOrArg0 fn_or_arg0, Args... args)
  {
    assert(!getConfig<Config>() &&
        "GLPipeline::add<State>() can be called for a given State only ONCE!");

    if constexpr(std::is_invocable_v<FnOrArg0, Config&>) { // The first argument matches void(Config&)
      static_assert(sizeof...(Args) == 0,
          "GLPipeline::add<Config>() accepts EITHER a callable or"
          "constructor arguments (both given)"
      );

      auto& config = m_configs[m_num_configs].emplace<Config>();

      fn_or_arg0(config);
    } else {      // Pass the arguments to the constructor
      m_configs[m_num_configs].emplace<Config>(
          std::forward<FnOrArg0>(fn_or_arg0), std::forward<Args>(args)...
      );
    }

    m_num_configs++;

    return *this;
  }

  template <typename Config, typename FnOrArg0, typename... Args>
  Pipeline& replace(FnOrArg0 fn_or_arg0, Args... args)
  {
    auto pconfig = getConfig<Config>();

    if(!pconfig)   // Fall back to add() if Config isn't already present
      return add<Config>(std::forward<FnOrArg0>(fn_or_arg0), std::forward<Args>(args)...);

    if constexpr(std::is_invocable_v<FnOrArg0, Config&>) {
      static_assert(sizeof...(Args) == 0,
          "GLPipeline::replace<Config>() accepts EITHER a callable or"
          "constructor arguments (both given)"
      );

      *pconfig = Config();    // Reset the Config to it's default state
      fn_or_arg0(*pconfig);
    } else {
      *pconfig = Config(std::forward<FnOrArg0>(fn_or_arg0), std::forward<Args>(args)...);
    }

    return *this;
  }

  Pipeline clone() const
  {
    return Pipeline(*this);
  }

  const Pipeline& use() const;

  Pipeline& draw(size_t offset, size_t num);
  Pipeline& draw(size_t num);

  Pipeline& drawBaseVertex(size_t base, size_t offset, size_t num);

private:
  using ConfigStructArray = std::array<ConfigStruct, std::variant_size_v<ConfigStruct>-2>;
  //                                                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //              Don't reserve slots for the null-ConfigStruct object and RawConfigStruct
  //
  // - Use a std::array instead of a std::vector to avoid dynamic memory allocations
  
  // The values MUST be arranged in accordance with the order
  //   of the StateStruct std::variant Types!
  enum class ConfigId {
    None,

    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,
    ClearColor, ClearDepth,
    SeamlessCubemap,
    Program,

    RawConfigStruct,
  };

  struct ConfigDiff {
    std::optional<ConfigStruct> current = std::nullopt;
    std::optional<ConfigStruct> next    = std::nullopt;
  };

  using ConfigDiffArray = std::array<ConfigDiff, std::variant_size_v<ConfigStruct>-2>;
  //                                             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //                                             See comment below ConfigStructArray

  static size_t sizeof_config_struct(const ConfigStruct& conf);
  static RawConfigStruct *raw_config(const ConfigStruct& conf);

  // Diffs 'm_configs' with 'other' and returns the differences
  ConfigDiffArray diff(const ConfigStructArray& other) const;

  void useConfig(const ConfigStruct& current, const ConfigStruct& next) const;

  void enableConfig(const ConfigStruct& config) const;
  void disableConfig(const ConfigStruct& config) const;

  template <typename Config>
  Config *getConfig()
  {
    Config *state = nullptr;
    for(size_t i = 0; i < m_num_configs; i++) {   // Iterate only the filled entries
      auto& s = m_configs[i];

      if(auto pstate = std::get_if<Config>(&s)) {
        state = pstate;
        break;
      }
    }

    return state;
  }

  ResourcePool *m_pool = nullptr;

  size_t m_num_configs = 0;   // Tracks how many Configs have been add()'ed to this
                              //   Pipeline so far (std::array has static size so
                              //   we must keep this number ourselves so it can be
                              //   used instead of a std::vector)

  ConfigStructArray m_configs;  // Initialized to { std::monostate, ... } in the ctor
};

class ScopedPipeline {
public:
  ScopedPipeline(const Pipeline& p);
  ~ScopedPipeline();

private:
  Pipeline m_previous;
};

}
