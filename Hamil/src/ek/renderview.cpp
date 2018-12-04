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

  // Driver issiues force this to be an ivec4
  ivec4 /* int */ num_lights;
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

  // Driver issiues force this to be an ivec4
  ivec4 /* int */ material_id;

  float metalness;
  float roughness;

  float pad_[2];
};

struct ObjectShadowConstants {
  mat4 model;
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
  m_constant_block_sz = (u32)gx::info().maxUniformBlockSize();
}

RenderView::~RenderView()
{
  for(auto rt : m_rts) renderer().releaseRenderTarget(*rt);

  if(m_renderpass_id != gx::ResourcePool::Invalid) m_pool->release<gx::RenderPass>(m_renderpass_id);
  if(m_scene_ubo_id != gx::ResourcePool::Invalid)  m_pool->releaseBuffer(m_scene_ubo_id);
  if(m_object_ubo_id != gx::ResourcePool::Invalid) m_pool->releaseBuffer(m_object_ubo_id);

  delete m_mempool;
}

RenderView& RenderView::depthOnlyRender()
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

const RenderView::RenderFn RenderView::RenderFns[NumViewTypes][NumRenderTypes] = {
  { nullptr, nullptr, nullptr },   // Invalid
  { nullptr, &RenderView::forwardCameraRenderOne, nullptr },   // CameraView
  { nullptr, nullptr, nullptr },   // LightView
  { &RenderView::shadowRenderOne, nullptr, nullptr },   // ShadowView
};

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

  auto scene_constants = generateSceneConstants();

  m_renderpass_id = createRenderPass();
  auto& renderpass = getRenderpass();

  initConstantBuffers(objects.size());

  auto cmd = gx::CommandBuffer::begin()
    .bindResourcePool(pool)
    .bindMemoryPool(m_mempool)
    .renderpass(m_renderpass_id)
    .bufferUpload(m_scene_ubo_id, scene_constants.h, scene_constants.sz);

  auto render_one = RenderFns[m_type][m_render];

  for(const auto& ro : objects) {
    (this->*render_one)(ro, cmd);
 
    advanceConstantBlockBinding(cmd);
  }

  return cmd.end();
}

const RenderTarget& RenderView::presentRenderTarget() const
{
  return *m_rts.at(0);
}

gx::Pipeline RenderView::createPipeline()
{
  auto pipeline = gx::Pipeline()
    .viewport(m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w)
    .depthTest(gx::LessEqual)
    .cull(gx::Pipeline::Back)
    .noBlend();

  if(m_render == DepthOnly && m_type != ShadowView) {
    pipeline.writeDepthOnly();
  }

  return pipeline;
}

u32 RenderView::createFramebuffer()
{
  RenderTargetConfig config;

  switch(m_render) {
  case DepthOnly:
    switch(m_type) {
    case CameraView: config = RenderTargetConfig::depth_prepass(m_samples); break;
    case ShadowView: config = RenderTargetConfig::msm_shadowmap(m_samples); break;
    }
    break;

  case Forward:
    config = RenderTargetConfig::forward_linearz(m_samples);
    break;

  case Deferred:
    break;

  default: assert(0); // unreachable
  }

  config.viewport = m_viewport;

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
    .pipeline(pipeline)
    .clearOp(gx::RenderPass::ClearColorDepth);

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

gx::RenderPass& RenderView::getRenderpass()
{
  return m_pool->get<gx::RenderPass>(m_renderpass_id);
}

void RenderView::initConstantBuffers(size_t num_ros)
{
  const u32 ObjectConstantsSize = constantBlockSizeAlign(sizeof(ObjectConstants));
  const u32 SceneConstantsSize  = constantBlockSizeAlign(sizeof(SceneConstants));

  // Unaligned size of ObjectConstants UniformBuffer
  const size_t ObjectConstantBufferUASize = num_ros * ObjectConstantsSize;

  // Must be a multiple of ConstantBlockMaxSize, so align it assuming it's
  //   unaligned (could waste some space in case that's false)
  const size_t ObjectConstantBufferSize = ObjectConstantBufferUASize +
    /* align */ (m_constant_block_sz - (ObjectConstantBufferUASize % m_constant_block_sz));

  m_scene_ubo_id  = createConstantBuffer(SceneConstantsSize);
  m_object_ubo_id = createConstantBuffer(ObjectConstantsSize);

  m_num_objects_per_block = m_constant_block_sz / ObjectConstantsSize;

  auto object_ubo = m_pool->getBuffer(m_object_ubo_id);
  object_ubo().init(ObjectConstantBufferSize, 1);

  auto object_ubo_view = object_ubo().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);

  m_objects = object_ubo_view.get<ObjectConstants>();
  m_objects_rover = StridePtr<ObjectConstants>(m_objects, ObjectConstantsSize);
  m_objects_end = (ObjectConstants *)((byte *)m_objects + ObjectConstantBufferSize);

  getRenderpass()
    .uniformBufferRange(SceneConstantsBinding, m_scene_ubo_id, 0, SceneConstantsSize)
    .uniformBufferRange(ObjectConstantsBinding, m_object_ubo_id, 0, m_constant_block_sz);
}

void RenderView::advanceConstantBlockBinding(gx::CommandBuffer& cmd)
{
  auto& renderpass = getRenderpass();

  auto current_rover = (uintptr_t)m_objects_rover.get();
  auto current_rover_off = current_rover - (uintptr_t)m_objects;

  // Check if we need to advance to the next uniform block yet
  if(current_rover_off % m_constant_block_sz != 0) return;

  // We need to advance to a new part of the UniformBuffer
  auto next_subpass = renderpass.nextSubpassId();
  auto subpass = gx::RenderPass::Subpass()
    .uniformBufferRange(ObjectConstantsBinding, m_object_ubo_id,
      current_rover_off, m_constant_block_sz);

  renderpass.subpass(subpass);
  cmd.subpass(next_subpass);
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

  scene->num_lights.x = 3;
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
void RenderView::forwardCameraRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants_offset = writeConstants(ro);

  auto program_id = renderer().queryProgram(*this, ro, *m_pool);
  auto& program = m_pool->get<gx::Program>(program_id);  // R.shader.shaders.forward

  if(m_init_programs.find(program_id) == m_init_programs.end()) {
    program.use()
      .uniformBlockBinding("SceneConstantsBlock", SceneConstantsBinding)
      .uniformBlockBinding("ObjectConstantsBlock", ObjectConstantsBinding)

      .uniformSampler(U.forward.uDiffuseTex, DiffuseTexImageUnit);

    m_init_programs.insert(program_id);
  }

  cmd
    .program(program_id)
    .uniformInt(U.forward.uObjectConstantsOffset, constants_offset);

  auto& renderpass = getRenderpass();

  const auto& material = ro.material();

  // TODO!
  //   - Batch RenderObject by Diffuse texture
  if(material.diff_type == hm::Material::DiffuseTexture) {
    auto next_subpass = renderpass.nextSubpassId();
    auto subpass = gx::RenderPass::Subpass()
      .texture(DiffuseTexImageUnit, material.diff_tex.id, material.diff_tex.sampler_id);

    renderpass.subpass(subpass);
    cmd.subpass(next_subpass);
  }

  emitDraw(ro, cmd);
}

void RenderView::shadowRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants_offset = writeConstants(ro);

  auto program_id = renderer().queryProgram(*this, ro, *m_pool);
  auto& program = m_pool->get<gx::Program>(program_id);  // R.shader.shaders.msm

  if(m_init_programs.find(program_id) == m_init_programs.end()) {
    program.use()
      .uniformBlockBinding("SceneConstantsBlock", SceneConstantsBinding)
      .uniformBlockBinding("ObjectConstantsBlock", ObjectConstantsBinding);

    m_init_programs.insert(program_id);
  }

  cmd
    .program(program_id)
    .uniformInt(U.msm.uObjectConstantsOffset, constants_offset);

  emitDraw(ro, cmd);
}

void RenderView::emitDraw(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  const auto& mesh = ro.mesh().m;

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