#include <gx/pipeline.h>
#include <gx/resourcepool.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <math/geometry.h>

#include <cassert>
#include <cstring>
#include <algorithm>

namespace gx {

auto p_current = Pipeline(nullptr);
/*
  .add<Pipeline::Scissor>([](auto& sc) {
      sc.no_test();
  });
  */

[[using gnu: always_inline]]
static constexpr auto blend_factor_to_GLEnum(u32 factor) -> GLenum
{
  switch(factor) {
  case Pipeline::Factor0:                 return GL_ZERO;
  case Pipeline::Factor1:                 return GL_ONE;
  case Pipeline::FactorSrcColor:          return GL_SRC_COLOR;
  case Pipeline::Factor1MinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
  case Pipeline::FactorDstColor:          return GL_DST_COLOR;
  case Pipeline::Factor1MinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
  case Pipeline::FactorSrcAlpha:          return GL_SRC_ALPHA;
  case Pipeline::Factor1MinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
  case Pipeline::FactorDstAlpha:          return GL_DST_ALPHA;
  case Pipeline::Factor1MinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
  case Pipeline::FactorConstColor:        return GL_CONSTANT_COLOR;
  case Pipeline::Factor1MinusConstColor:  return GL_ONE_MINUS_CONSTANT_COLOR;
  case Pipeline::FactorConstAlpha:        return GL_CONSTANT_ALPHA;
  case Pipeline::Factor1MinusConstAlpha:  return GL_ONE_MINUS_CONSTANT_ALPHA;
  case Pipeline::FactorSrcAlpha_Saturate: return GL_SRC_ALPHA_SATURATE;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto cull_mode_to_GLEnum(u32 mode) -> GLenum
{
  switch(mode) {
  case Pipeline::CullFront:        return GL_FRONT;
  case Pipeline::CullBack:         return GL_BACK;
  case Pipeline::CullFrontAndBack: return GL_FRONT_AND_BACK;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto front_face_to_GLEnum(u32 face) -> GLenum
{
  switch(face) {
  case Pipeline::FrontFaceCCW: return GL_CCW;
  case Pipeline::FrontFaceCW:  return GL_CW;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto poly_mode_to_GLEnum(u32 mode) -> GLenum
{
  switch(mode) {
  case Pipeline::PolygonModeFilled: return GL_FILL;
  case Pipeline::PolygonModeLines:  return GL_LINE;
  case Pipeline::PolygonModePoints: return GL_POINT;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto blend_func_to_GLEnum(u32 func) -> GLenum
{
  switch(func) {
  case Pipeline::BlendFuncAdd: return GL_FUNC_ADD;
  case Pipeline::BlendFuncSub: return GL_FUNC_SUBTRACT;
  case Pipeline::BlendFuncMin: return GL_MIN;
  case Pipeline::BlendFuncMax: return GL_MAX;
  }

  return GL_INVALID_ENUM;
}

Pipeline::Pipeline(ResourcePool *pool) :
  m_pool(pool)
{
  std::fill(m_configs.begin(), m_configs.end(), std::monostate());
}

const Pipeline& Pipeline::use() const
{
  for(const auto& c : diff(p_current.m_configs)) {
    auto has_config   = c.current.has_value(),
         wants_config = c.next.has_value();

    if(!has_config && !wants_config) break;  // diff() compacts the resulting array, so
                                             //   we can early-out at the first empty
                                             //   ConfigDiff

    if(has_config && !wants_config) {     // Config must be disabled
      disableConfig(c.current.value());
    } else if(!has_config && wants_config) {  // Config must be enabled
      enableConfig(c.next.value());
    } else {   // The parameters have changed
      useConfig(c.current.value(),  c.next.value());
    }
  }

  return p_current = *this;
}

Pipeline& Pipeline::draw(size_t offset, size_t num)
{
  assert(m_pool && "a Pipeline MUST have a ResourcePool bound to draw() with it!");

  assert(getConfig<VertexInput>() && getConfig<InputAssembly>() &&
      "a Pipeline MUST have VertexInput and InputAssembly configs to call draw()!");

  auto& vi = *getConfig<VertexInput>();
  auto& ia = *getConfig<InputAssembly>();

  assert(vi.array != ~0u && "VertexInput.with_array() not set before draw()!");

  assert(ia.primitive != gx::Primitive::Invalid &&
      "invalid InputAssembly config bound to Pipeline! (missing 'primitive')");

  auto prim = gl_prim(ia.primitive);

  if(auto program_conf = getConfig<Program>()) {
    auto& program = m_pool->get<gx::Program>(program_conf->id);

    program.use();
  } else {
    // TODO: bind NULL program here...
  }

  if(!vi.isIndexed()) {
    auto& vtx = m_pool->get<VertexArray>(vi.array);

    vtx.use();
    glDrawArrays(prim, (int)offset, (GLsizei)num);
  } else {
    auto& vtx  = m_pool->get<IndexedVertexArray>(vi.array);
    auto idx_type = gl_type(vtx.indexType());

    auto off = (void *)(offset * vtx.indexSize());

    vtx.use();
    glDrawElements(prim, (GLsizei)num, idx_type, off);
  }

  return *this;
}

Pipeline& Pipeline::draw(size_t num)
{
  return draw(0, num);
}

Pipeline& Pipeline::drawBaseVertex(size_t base, size_t offset, size_t num)
{
  assert(m_pool && "a Pipeline MUST have a ResourcePool bound to draw() with it!");

  assert(getConfig<VertexInput>() && getConfig<InputAssembly>() &&
      "a Pipeline MUST have VertexInput and InputAssembly configs to call draw()!");

  auto& vi = *getConfig<VertexInput>();
  auto& ia = *getConfig<InputAssembly>();

  assert(vi.array != ~0u && "VertexInput.with_array() not set before drawBaseVertex()!");

  assert(vi.isIndexed() &&
      "drawBaseVertex() can only be called with indexed VertexInputs!");

  assert(ia.primitive != gx::Primitive::Invalid &&
      "invalid InputAssembly config bound to Pipeline! (missing 'primitive')");

  auto prim = gl_prim(ia.primitive);

  if(auto program_conf = getConfig<Program>()) {
    auto& program = m_pool->get<gx::Program>(program_conf->id);

    program.use();
  } else {
    // TODO: bind NULL program here...
  }

  auto& vtx = m_pool->get<IndexedVertexArray>(vi.array);
  auto idx_type = gl_type(vtx.indexType());

  auto off = (void *)(offset * vtx.indexSize());

  vtx.use();
  glDrawElementsBaseVertex(prim, (GLsizei)num, idx_type, off, (int)base);

  return *this;
}

Pipeline Pipeline::current()
{
  return p_current;
}

size_t Pipeline::sizeof_config_struct(const ConfigStruct& conf)
{
  switch((ConfigId)conf.index()) {
  case ConfigId::None:            return 0;
  case ConfigId::VertexInput:     return sizeof(VertexInput);
  case ConfigId::InputAssembly:   return sizeof(InputAssembly); break;
  case ConfigId::Viewport:        return sizeof(Viewport);
  case ConfigId::Scissor:         return sizeof(Scissor);
  case ConfigId::Rasterizer:      return sizeof(Rasterizer);
  case ConfigId::DepthStencil:    return sizeof(DepthStencil);
  case ConfigId::Blend:           return sizeof(Blend);
  case ConfigId::ClearColor:      return sizeof(ClearColor);
  case ConfigId::ClearDepth:      return sizeof(ClearDepth);
  case ConfigId::SeamlessCubemap: return sizeof(SeamlessCubemap);
  case ConfigId::Program:         return sizeof(Program);
  }

  return (size_t)~0u;
}

template <typename Config>
static Pipeline::RawConfigStruct *get_ptr(const Pipeline::ConfigStruct& conf)
{
  return (Pipeline::RawConfigStruct *)&std::get<Config>(conf);
}

auto Pipeline::raw_config(const ConfigStruct& conf) -> RawConfigStruct *
{
  RawConfigStruct *raw = nullptr;

  switch((ConfigId)conf.index()) {
  case ConfigId::VertexInput:     raw = get_ptr<VertexInput>(conf); break;
  case ConfigId::InputAssembly:   raw = get_ptr<InputAssembly>(conf); break;
  case ConfigId::Viewport:        raw = get_ptr<Viewport>(conf); break;
  case ConfigId::Scissor:         raw = get_ptr<Scissor>(conf); break;
  case ConfigId::Rasterizer:      raw = get_ptr<Rasterizer>(conf); break;
  case ConfigId::DepthStencil:    raw = get_ptr<DepthStencil>(conf); break;
  case ConfigId::Blend:           raw = get_ptr<Blend>(conf); break;
  case ConfigId::ClearColor:      raw = get_ptr<ClearColor>(conf); break;
  case ConfigId::ClearDepth:      raw = get_ptr<ClearDepth>(conf); break;
  case ConfigId::SeamlessCubemap: raw = get_ptr<SeamlessCubemap>(conf); break;
  case ConfigId::Program:         raw = get_ptr<Program>(conf); break;
  }

  return raw;
}

auto Pipeline::diff(const ConfigStructArray& other) const -> ConfigDiffArray
{
  auto order_configs = [](const ConfigStructArray& array) -> ConfigStructArray {
    ConfigStructArray ordered;

    std::fill(ordered.begin(), ordered.end(), std::monostate());
    for(const auto& conf : array) {
      if(std::holds_alternative<std::monostate>(conf)) continue;

      ordered[conf.index() - 1] = conf;  // ConfigId::None (the null ConfigStruct) does
                                         //  not have a slot reserved in ConfigStructArray
                                         //  so shift all Config's back by one
    }

    return ordered;
  };

  ConfigStructArray other_ordered = order_configs(other);
  ConfigStructArray self_ordered  = order_configs(m_configs);

  ConfigDiffArray difference;
  std::fill(difference.begin(), difference.end(), ConfigDiff());

  for(size_t i = 0; i < difference.size(); i++) {
    const auto& conf_other = other_ordered[i];
    const auto& conf_self  = self_ordered[i];

    const auto conf_other_ptr = raw_config(conf_other);
    const auto conf_self_ptr  = raw_config(conf_self);

    auto other_sz = sizeof_config_struct(conf_other);
    auto self_sz  = sizeof_config_struct(conf_self);

    bool confs_differ = true;   // Assume self_sz != other_sz => thus the Configs differ
    if(self_sz == other_sz) {
      // The sizes will be different when we're either enabling or disabling
      //   a Config, as then one of self/other will have ConfigId::None
      confs_differ = memcmp(conf_other_ptr, conf_self_ptr, self_sz);
    }

    // The structs are equal - so move on
    if(!confs_differ) continue;

    // Otherwise - store the difference
    ConfigDiff d;

    // d.current and d.next are std::nullopt by default, which
    //   indicates that a given Config isn't present in the current
    //   Pipeline nor the 'other' Pipeline
    // If however the Config has been added to the current Pipeline,
    //   removed from it or altered between the Pipelines store
    //   the current Config, the other Config or both
    if((ConfigId)conf_self.index() != ConfigId::None) d.next.emplace(conf_self);
    if((ConfigId)conf_other.index() != ConfigId::None) d.current.emplace(conf_other);

    difference[i] = d;
  }

  // Compact the result (move all filled ConfigStructs to the beginning)
  std::remove_if(difference.begin(), difference.end(), [](const ConfigDiff& d) {
      return !d.current.has_value() && !d.next.has_value();
  });
  
  return difference;
}

// Needed for std::visit on ConfigStruct
template <typename... Configs>
struct overloaded : Configs... {
  using Configs::operator()...;
};

template <typename... Configs>
overloaded(Configs...) -> overloaded<Configs...>;

void Pipeline::useConfig(const ConfigStruct& current, const ConfigStruct& next) const
{
  auto has_config = (ConfigId)current.index() != ConfigId::None;

  assert(!has_config || (current.index() == next.index()));

  std::visit(
    overloaded {
      [&,this](const InputAssembly& ia) {
        const auto& current_ia = std::get<InputAssembly>(current);
        if(current_ia.primitive_restart == ia.primitive_restart &&
            current_ia.restart_index == ia.restart_index) {
          // Only ia.primitive must've changed - so no need to
          //   touch GL_PRIMITIVE_RESTART or glPrimitiveRestartIndex()
          return;
        }

        enableConfig(ia);
      },
      [](const Viewport& v) {
        glViewport(v.x, v.y, v.w, v.h);
      },
      [&](const Scissor& sc) {
        const auto& current_sc = std::get<Scissor>(current);
        // Check if we need to enable/disable the scissor test...
        if(current_sc.scissor != sc.scissor) {
          if(sc.scissor) {
            glEnable(GL_SCISSOR_TEST);
          } else {
            glDisable(GL_SCISSOR_TEST);

            return;
          }
        }

        glScissor(sc.x, sc.y, sc.w, sc.h);    // ...or only update the scissor rect
      },
      [&](const Rasterizer& r) {
        const auto& current_r = std::get<Rasterizer>(current);
        if(current_r.cull_mode != r.cull_mode) {
          if(r.cull_mode == CullNone) {
            glDisable(GL_CULL_FACE);
          } else {
            glEnable(GL_CULL_FACE);

            auto mode = cull_mode_to_GLEnum(r.cull_mode);
            glCullFace(mode);
          }
        }

        if(current_r.front_face != r.front_face) {
          auto front_face = front_face_to_GLEnum(r.front_face);
          glFrontFace(front_face);
        }

        if(current_r.polygon_mode != r.polygon_mode) {
          auto poly_mode = poly_mode_to_GLEnum(r.polygon_mode);
          glPolygonMode(GL_FRONT_AND_BACK, poly_mode);
        }
      },
      [&](const Blend& b) {
        const auto& current_b = std::get<Blend>(current);
        if(current_b.blend != b.blend) {
          if(b.blend) {
            glEnable(GL_BLEND);
          } else {
            glDisable(GL_BLEND);
          }
        }

        if(current_b.func != b.func) {
          auto func = blend_func_to_GLEnum(b.func);

          glBlendEquation(func);
        }

        if(current_b.src_factor != b.src_factor ||
            current_b.dst_factor != b.dst_factor) {
          auto sfactor = blend_factor_to_GLEnum(b.src_factor),
               dfactor = blend_factor_to_GLEnum(b.dst_factor);

          glBlendFunc(sfactor, dfactor);
        }
      },
      [&](const ClearColor& cc) {
        const auto& current_cc = std::get<ClearColor>(current);

        auto [cr, cg, cb, ca] = current_cc;
        auto [r, g, b, a]     = cc;

        if(vec4(cr, cg, cb, ca) == vec4(r, g, b, a)) return;

        glClearColor(r, g, b, a);
      },
      [&](const ClearDepth& cd) {
        const auto& current_cd = std::get<ClearDepth>(current);
        if(current_cd.z == cd.z) return;

        glClearDepth(cd.z);
      },
      [](const SeamlessCubemap& cubemap) {
        if(cubemap.seamless) {
          glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        } else {
          glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
      },
      [this](const Program& p) {
        assert(m_pool && "Pipeline must have valid ResourcePool to have a Program bound!");

        auto& program = m_pool->get<gx::Program>(p.id);

        program.use();
      },

      [](auto c) { }     // Default
    }, next
  );

}

void Pipeline::enableConfig(const ConfigStruct& config) const
{
  std::visit(
    overloaded {
      [&](const InputAssembly& ia) {
        if(ia.primitive_restart) {
          glEnable(GL_PRIMITIVE_RESTART);

          glPrimitiveRestartIndex(ia.restart_index);
        } else {
          glDisable(GL_PRIMITIVE_RESTART);
        }
      },
      [](const Scissor& sc) {
        if(sc.scissor) {
          glEnable(GL_SCISSOR_TEST);

          glScissor(sc.x, sc.y, sc.w, sc.h);
        } else {
          glDisable(GL_SCISSOR_TEST);
        }
      },
      [](const Rasterizer& r) {
        if(r.cull_mode == CullNone) {
          glDisable(GL_CULL_FACE);
        } else {
          glEnable(GL_CULL_FACE);

          auto mode = cull_mode_to_GLEnum(r.cull_mode);
          glCullFace(mode);
        }

        auto front_face = front_face_to_GLEnum(r.front_face);
        glFrontFace(front_face);

        auto poly_mode = poly_mode_to_GLEnum(r.polygon_mode);
        glPolygonMode(GL_FRONT_AND_BACK, poly_mode);
      },
      [&](const DepthStencil& ds) {
        if(ds.depth_test) {
          glEnable(GL_DEPTH_TEST);
        } else {
          glDisable(GL_DEPTH_TEST);
        }

        switch(ds.depth_func) {
        case CompareFuncNever:        glDepthFunc(GL_NEVER); break;
        case CompareFuncAlways:       glDepthFunc(GL_ALWAYS); break;
        case CompareFuncEqual:        glDepthFunc(GL_EQUAL); break;
        case CompareFuncNotEqual:     glDepthFunc(GL_NOTEQUAL); break;
        case CompareFuncLess:         glDepthFunc(GL_LESS); break;
        case CompareFuncLessEqual:    glDepthFunc(GL_LEQUAL); break;
        case CompareFuncGreater:      glDepthFunc(GL_GREATER); break;
        case CompareFuncGreaterEqual: glDepthFunc(GL_GEQUAL); break;
        }
      },
      [](const Blend& b) {
        if(b.blend) {
          glEnable(GL_BLEND);

          auto sfactor = blend_factor_to_GLEnum(b.src_factor),
               dfactor = blend_factor_to_GLEnum(b.dst_factor);

          glBlendEquation(GL_FUNC_ADD);
          glBlendFunc(sfactor, dfactor);
        } else {
          glDisable(GL_BLEND);
        }
      },
      [](const ClearColor& cc) {
        auto [r, g, b, a] = cc;

        glClearColor(r, g, b, a);
      },
      [](const ClearDepth& cd) {
        glClearDepth(cd.z);
      },

      [this](auto c) {
        useConfig(std::monostate(), c);
      }    // Default case
    },  config
  );
}

void Pipeline::disableConfig(const ConfigStruct& config) const
{
  std::visit(
    overloaded {
      [](const VertexInput& vi) {
      },
      [](const InputAssembly& ia) {
        glDisable(GL_PRIMITIVE_RESTART);
      },
      [](const Scissor& sc) {
        glDisable(GL_SCISSOR_TEST);
      },
      [](const DepthStencil& ds) {
        glDisable(GL_DEPTH_TEST);
      },
      [](const Blend& b) {
        glDisable(GL_BLEND);
      },
      [](const SeamlessCubemap& cubemap) {
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      },
      [this](const Program& p) {
        // TODO: call glUseProgram(0) here somehow...
      },

      [](auto c) { }    // Default case
    },  config
  );
}

ScopedPipeline::ScopedPipeline(const Pipeline& p) :
  m_previous(Pipeline::current())
{
  p.use();
}

ScopedPipeline::~ScopedPipeline()
{
  m_previous.use();
}

}
