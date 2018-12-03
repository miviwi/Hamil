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

#include <uniforms.h>

#include <cassert>
#include <cstring>
#include <algorithm>

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

enum MaterialId : u32 {
  Unshaded,
  ConstantColor,
  ProceduralColor,
  Textured,
};

#pragma pack(push, 1)
struct LightConstants {
  vec4 /* vec3 */ position;
  vec4 /* vec3 */ color;
};

struct SceneConstants {
  mat4 view;
  mat4 projection;

  // projection * view
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

  ivec4 material_id;

  float metalness;
  float roughness;

  float pad_[2];
};
#pragma pack(pop)

RenderView::RenderView(ViewType type) :
  m_type(type),
  m_render((RenderType)~0u),
  m_viewport(0, 0, 0, 0),
  m_samples(0),
  m_view(mat4::identity()), m_projection(mat4::identity()),
  m_renderer(nullptr),
  m_mempool(nullptr), m_pool(nullptr),
  m_renderpass_id(gx::ResourcePool::Invalid),
  m_scene_ubo_id(gx::ResourcePool::Invalid),
  m_object_ubo_id(gx::ResourcePool::Invalid),
  m_objects_rover(nullptr, 1)
{
  m_ubo_alignment = pow2_round((uint)gx::info().minUniformBindAlignment());
}

RenderView::~RenderView()
{
  for(auto rt : m_rts) renderer().releaseRenderTarget(*rt);

  if(m_renderpass_id != gx::ResourcePool::Invalid) m_pool->release<gx::RenderPass>(m_renderpass_id);
  if(m_scene_ubo_id != gx::ResourcePool::Invalid)  m_pool->releaseBuffer(m_scene_ubo_id);
  if(m_object_ubo_id != gx::ResourcePool::Invalid) m_pool->releaseBuffer(m_object_ubo_id);

  delete m_mempool;
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
  gx::ResourcePool *pool)
{
  // Used by internal methods
  m_renderer = &renderer;

  // Used by internal methods
  m_pool = pool;
  // Internal to this RenderView
  m_mempool = new gx::MemoryPool(MempoolInitialAlloc);

  const u32 ObjectConstantsSize = constantBlockSizeAlign(sizeof(ObjectConstants));
  const u32 SceneConstantsSize  = constantBlockSizeAlign(sizeof(SceneConstants));
  const size_t ConstantBlockMaxSize = gx::info().maxUniformBlockSize();

  // Unaligned size of ObjectConstants UniformBuffer
  const size_t ObjectConstantBufferUASize = objects.size() * ObjectConstantsSize;

  // Must be a multiple of ConstantBlockMaxSize, so align it assuming it's
  //   unaligned (could waste some space in case that's false)
  const size_t ObjectConstantBufferSize = ObjectConstantBufferUASize +
    /* align */ (ConstantBlockMaxSize - (ObjectConstantBufferUASize % ConstantBlockMaxSize));

  m_scene_ubo_id  = createConstantBuffer(SceneConstantsSize);
  m_object_ubo_id = createConstantBuffer(ObjectConstantsSize);

  m_num_objects_per_block = ConstantBlockMaxSize / ObjectConstantsSize;

  auto scene_constants = generateSceneConstants();

  auto object_ubo = m_pool->getBuffer(m_object_ubo_id);
  object_ubo().init(ObjectConstantBufferSize, 1);

  auto object_ubo_view = object_ubo().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);

  m_objects = object_ubo_view.get<ObjectConstants>();
  m_objects_rover = StridePtr<ObjectConstants>(m_objects, ObjectConstantsSize);
  m_objects_end = (ObjectConstants *)((byte *)m_objects + objects.size()*ObjectConstantsSize);

  m_renderpass_id = createRenderPass();
  auto& renderpass = m_pool->get<gx::RenderPass>(m_renderpass_id);

  renderpass
    .uniformBufferRange(SceneConstantsBinding, m_scene_ubo_id, 0, SceneConstantsSize)
    .uniformBufferRange(ObjectConstantsBinding, m_object_ubo_id, 0, ConstantBlockMaxSize);

  auto cmd = gx::CommandBuffer::begin()
    .bindResourcePool(pool)
    .bindMemoryPool(m_mempool)
    .renderpass(m_renderpass_id)
    .bufferUpload(m_scene_ubo_id, scene_constants.h, scene_constants.sz);

  for(const auto& ro : objects) {
    renderOne(ro, cmd);

    auto current_rover = (uintptr_t)m_objects_rover.get();
    auto current_rover_off = current_rover - (uintptr_t)m_objects;

    if(current_rover_off % ConstantBlockMaxSize == 0) {
      // We need to advance to a new part of the UniformBuffer
      auto next_subpass = renderpass.nextSubpassId();

      renderpass.subpass(gx::RenderPass::Subpass()
        .uniformBufferRange(ObjectConstantsSize, m_object_ubo_id, current_rover_off, ConstantBlockMaxSize)
      );

      cmd.subpass(next_subpass);
    }
  }

  return cmd.end();
}

const RenderTarget& RenderView::renderTarget(uint index) const
{
  return *m_rts.at(index);
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
  RenderTargetConfig config;

  switch(m_render) {
  case DepthOnly:
    config = RenderTargetConfig::depth_prepass(m_samples);
    break;

  case Forward:
    config = RenderTargetConfig::forward_linearz(m_samples);
    break;

  case Deferred:
    break;

  default: assert(0); // unreachable
  }

  config.viewport = { 0, 0, 1280, 720 }; // TODO

  const auto& rt = renderer().queryRenderTarget(config, *m_pool);
  m_rts.emplace_back(&rt);

  return rt.framebufferId();
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

u32 RenderView::createConstantBuffer(u32 sz)
{
  auto id = m_pool->createBuffer<gx::UniformBuffer>(gx::Buffer::Dynamic);
  auto buf = m_pool->getBuffer(id);

  buf().init(constantBlockSizeAlign(sz), 1);

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

  memset(scene->light_types, 0, sizeof(SceneConstants::light_types));

  scene->num_lights = 3;
  scene->lights[0] = {
    m_view * vec4(0.0f, 6.0f, 0.0f, 1.0f),
    vec3{ 1.0f, 1.0f, 1.0f }
  };
  scene->lights[1] = {
    m_view * vec4(-10.0f, 6.0f, -10.0f, 1.0f),
    vec3{ 1.0f, 1.0f, 0.0f }
  };
  scene->lights[2] = {
    m_view * vec4(20.0f, 6.0f, 0.0f, 1.0f),
    vec3{ 0.0f, 1.0f, 1.0f }
  };

  memcpy(scene->ambient_basis, ambient_basis, sizeof(SceneConstants::ambient_basis));

  return constants;
}

u32 RenderView::writeConstants(const RenderObject& ro)
{
  assert(m_objects_rover.get() < m_objects_end && "Wrote too many ObjectConstants!");

  // Move the rover forward
  ObjectConstants& object = *m_objects_rover++;

  const auto& model_matrix = ro.model();
  const auto& material = ro.material();

  object.model   = model_matrix;
  object.normal  = model_matrix.inverse().transpose();
  object.texture = mat4::identity();

  switch(material.diff_type) {
  case hm::Material::DiffuseConstant: object.material_id.x = ConstantColor; break;
  case hm::Material::DiffuseTexture:  object.material_id.x = Textured; break;
  case hm::Material::Other:           object.material_id.x = ProceduralColor; break;

  default: object.material_id.x = Unshaded; break;
  }

  object.diff_color = material.diff_color;
  object.ior = vec4(material.ior, 1.0f);
  object.metalness = material.metalness;
  object.roughness = material.roughness;

  size_t buffer_off = &object - m_objects;

  // The buffer is divided into blocks where one block
  //   has the max possible size bindable at once by the GPU,
  //   thus we need to extract the offset of the object in the
  //   CURRENTLY BOUND block
  u32 block_off = (u32)(buffer_off % m_num_objects_per_block);

  return block_off;
}

// TODO
void RenderView::renderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants_offset = writeConstants(ro);

  auto program_id = renderer().queryProgram(ro, *m_pool);
  auto& program = m_pool->get<gx::Program>(program_id);

  program
    .uniformBlockBinding("SceneConstantsBlock", SceneConstantsBinding)
    .uniformBlockBinding("ObjectConstantsBlock", ObjectConstantsBinding);

  cmd
    .program(program_id)
    .uniformSampler(U.forward.uDiffuseTex, DiffuseTexImageUnit)
    .uniformInt(U.forward.uObjectConstantsOffset, constants_offset)
    ;

  auto& renderpass = m_pool->get<gx::RenderPass>(m_renderpass_id);

  const auto& mesh = ro.mesh().m;
  const auto& material = ro.material();

  // TODO!
  if(material.diff_type == hm::Material::DiffuseTexture) {
    auto next_subpass = renderpass.nextSubpassId();

    renderpass.subpass(gx::RenderPass::Subpass()
      .texture(DiffuseTexImageUnit, material.diff_tex.id, material.diff_tex.sampler_id)
    );

    cmd.subpass(next_subpass);
  }

  if(mesh.isIndexed()) {
    if(mesh.base != mesh::Mesh::None && mesh.offset != mesh::Mesh::None) {
      cmd.drawBaseVertex(mesh.getPrimitive(), mesh.vertex_array_id,
        mesh.num, mesh.base, mesh.offset);
    } else { // Use the shorter command when possible
      cmd.drawIndexed(mesh.getPrimitive(), mesh.vertex_array_id, mesh.num);
    }
  } else {
    cmd.draw(mesh.getPrimitive(), mesh.vertex_array_id, mesh.num);
  }
}

}