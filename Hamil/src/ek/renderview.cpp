#include <ek/renderview.h>
#include <ek/renderer.h>
#include <ek/renderobject.h>

#include <math/util.h>
#include <gx/gx.h>
#include <gx/info.h>
#include <gx/commandbuffer.h>
#include <gx/resourcepool.h>
#include <gx/memorypool.h>
#include <gx/pipeline.h>
#include <gx/renderpass.h>
#include <gx/vertex.h>
#include <gx/texture.h>

#include <cassert>
#include <cstring>

namespace ek {

struct ShaderConstants {
  gx::MemoryPool::Handle h = gx::MemoryPool::Invalid;
  uint sz = ~0u;
};

enum : uint {
  MaxForwardPassLights = 8,
};

enum LightType : uint {
  Directional, Spot, Point,
};

enum MaterialId : uint {
  Unshaded,
  ConstantColor,
  ProceduralColor,
  Textured,
};

struct LightConstants {
  vec4 /* vec3 */ position;
  vec4 /* vec3 */ color;
};

struct SceneConstants {
  mat4 view;
  mat4 projection;

  mat4 viewprojection;

  vec4 /* vec3 */ ambient_basis[6];

  uint num_lights;
  // - Each vector stores 4 adjacent LightTypes which
  //   correspond to the lights[] array
  // - Packed to save memory used for alignment padding
  ivec4 light_types[MaxForwardPassLights / 4];
  LightConstants lights[MaxForwardPassLights];
};

struct ObjectConstants {
  mat4 model;
  mat4 normal;
  mat4 texture;

  vec4 /* vec3 */ diff_color;
  vec4 /* vec3 */ ior;

  MaterialId material_id;

  float metalness;
  float roughness;
};

RenderView::RenderView(ViewType type) :
  m_type(type),
  m_render((RenderType)~0u),
  m_viewport(0, 0, 0, 0),
  m_samples(0),
  m_view(mat4::identity()), m_projection(mat4::identity()),
  m_mempool(nullptr), m_pool(nullptr),
  m_num_rts(0),
  m_object_ubo_id(gx::ResourcePool::Invalid)
{
  m_ubo_alignment = pow2_round((uint)gx::info().minUniformBindAlignment());
}

RenderView& RenderView::depthPrepass()
{
  m_render = DepthOnly;

  return *this;
}

RenderView& RenderView::forwardRender()
{
  m_render = Forward;

  return *this;
}

RenderView& RenderView::deferredRender()
{
  m_render = Deferred;

  return *this;
}

RenderView& RenderView::viewport(ivec2 viewport)
{
  m_viewport = { 0, 0, viewport.x, viewport.y };

  return *this;
}

RenderView& RenderView::sampleCount(uint samples)
{
  m_samples = samples;

  return *this;
}

RenderView& RenderView::view(const mat4& v)
{
  m_view = v;

  return *this;
}

RenderView& RenderView::projection(const mat4& p)
{
  m_projection = p;

  return *this;
}

frustum3 RenderView::constructFrustum()
{
  return frustum3(m_view, m_projection);
}

gx::CommandBuffer RenderView::render(Renderer& renderer,
  const std::vector<RenderObject>& objects,
  gx::MemoryPool *mempool, gx::ResourcePool *pool)
{
  m_renderer = &renderer;

  m_pool = pool;
  m_mempool = mempool;

  auto renderpass_id = createRenderPass();

  auto scene_constants = generateSceneConstants();

  auto cmd = gx::CommandBuffer::begin()
    .bindResourcePool(pool)
    .bindMemoryPool(mempool)
    .renderpass(renderpass_id)
    .bufferUpload(m_scene_ubo_id, scene_constants.h, scene_constants.sz);

  for(const auto& ro : objects) {
    renderOne(ro, cmd);
  }

  return cmd.end();
}

gx::Pipeline RenderView::createPipeline()
{
  auto pipeline = gx::Pipeline()
    .viewport(m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w)
    .depthTest(gx::LessEqual)
    .cull(gx::Pipeline::Back)
    .noBlend();

  if(m_render == DepthOnly) {
    pipeline.writeDepthOnly();
  }

  return pipeline;
}

u32 RenderView::createFramebuffer()
{
  if(m_samples == 0) return doCreateFramebufferNoMultisample();

  return doCreateFramebufferMultisample();
}

u32 RenderView::createRenderPass()
{
  auto id = m_pool->create<gx::RenderPass>();

  auto framebuffer_id = createFramebuffer();
  auto pipeline = createPipeline();

  auto& pass = m_pool->get<gx::RenderPass>(id);

  pass
    .framebuffer(framebuffer_id)
    .pipeline(pipeline);

  switch(m_render) {
  case DepthOnly:
    pass
      .clearOp(gx::RenderPass::ClearDepth);
    break;

  case Forward:
  case Deferred:
    pass
      .clearOp(gx::RenderPass::ClearColorDepth);
    break;

  default: assert(0); // unreachable
  }

  return id;
}

u32 RenderView::doCreateFramebufferNoMultisample()
{
  auto id = m_pool->create<gx::Framebuffer>();
  auto& fb = m_pool->get<gx::Framebuffer>(id);

  auto create_rt = [this](gx::Format fmt) -> gx::Texture2D& {
    auto rt_id = m_pool->createTexture<gx::Texture2D>(fmt);
    auto rt = m_pool->getTexture(rt_id);

    rt().init(m_viewport.z, m_viewport.w);

    m_rts[m_num_rts] = rt_id;
    m_num_rts++;

    return rt.get<gx::Texture2D>();
  };

  switch(m_render) {
  case DepthOnly:
    fb.use()
      .renderbuffer(gx::depth32f, gx::Framebuffer::Depth);
    break;

  case Forward:
    fb.use()
      .tex(create_rt(gx::rgb8), 0, gx::Framebuffer::Color(0))  /* Accumulation */
      .tex(create_rt(gx::r32f), 0, gx::Framebuffer::Color(1))  /* Linear Z */
      .renderbuffer(gx::depth16, gx::Framebuffer::Depth);
    break;

  case Deferred:
    break;

  default: assert(0); // unreachable
  }

  return id;
}

u32 RenderView::doCreateFramebufferMultisample()
{
  auto id = m_pool->create<gx::Framebuffer>();
  auto& fb = m_pool->get<gx::Framebuffer>(id);

  auto create_rt = [this](gx::Format fmt) -> gx::Texture2D& {
    auto rt_id = m_pool->createTexture<gx::Texture2D>(fmt, gx::Texture::Multisample);
    auto& rt = m_pool->getTexture<gx::Texture2D>(rt_id);

    rt.initMultisample(m_samples, m_viewport.z, m_viewport.w);

    m_rts[m_num_rts] = rt_id;
    m_num_rts++;

    return rt;
  };

  switch(m_render) {
  case DepthOnly:
    fb.use()
      .renderbufferMultisample(m_samples, gx::depth32f, gx::Framebuffer::Depth);
    break;

  case Forward:
    fb.use()
      .tex(create_rt(gx::rgb8), 0, gx::Framebuffer::Color(0))  /* Accumulation */
      .tex(create_rt(gx::r32f), 0, gx::Framebuffer::Color(1))  /* Linear Z */
      .renderbuffer(gx::depth16, gx::Framebuffer::Depth);
    break;

  case Deferred:
    break;

  default: assert(0); // unreachable
  }

  return id;
}

u32 RenderView::constantBlockSizeAlign(u32 sz)
{
  return pow2_align(sz, m_ubo_alignment);
}

ShaderConstants RenderView::generateSceneConstants()
{
  ShaderConstants constants;

  constants.sz = constantBlockSizeAlign(sizeof(SceneConstants));
  constants.h  = m_mempool->alloc(constants.sz);

  SceneConstants *scene = m_mempool->ptr<SceneConstants>(constants.h);

  scene->view = m_view;
  scene->projection = m_projection;
  scene->viewprojection = m_projection * m_view;

  // TODO: pass these in as a parameter...
  vec4 ambient_basis[6] = {
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
  };

  scene->num_lights = 0;
  memset(scene->light_types, 0, sizeof(SceneConstants::light_types));
  memset(scene->lights, 0, sizeof(SceneConstants::lights));

  memcpy(scene->ambient_basis, ambient_basis, sizeof(SceneConstants::ambient_basis));

  return constants;
}

ShaderConstants RenderView::generateConstants(const RenderObject& ro)
{
  ShaderConstants constants;

  constants.sz = constantBlockSizeAlign(sizeof(ObjectConstants));
  constants.h  = m_mempool->alloc(constants.sz);

  const auto& model_matrix = ro.model();
  const auto& material = ro.material();
  ObjectConstants *object = m_mempool->ptr<ObjectConstants>(constants.h);

  object->model   = model_matrix;
  object->normal  = model_matrix.inverse().transpose();
  object->texture = mat4::identity();

  switch(material.diff_type) {
  case hm::Material::DiffuseConstant: object->material_id = ConstantColor; break;
  case hm::Material::DiffuseTexture:  object->material_id = Textured; break;
  case hm::Material::Other:           object->material_id = ProceduralColor; break;

  default: object->material_id = Unshaded; break;
  }

  object->diff_color = material.diff_color;
  object->ior = vec4(material.ior, 1.0f);
  object->metalness = material.metalness;
  object->roughness = material.roughness;

  return constants;
}

// TODO
u32 RenderView::getProgram(const RenderObject & ro)
{
  return m_programs.front();
}

void RenderView::renderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants = generateConstants(ro);

  cmd
    .bufferUpload(m_object_ubo_id, constants.h, constants.sz);
}

}