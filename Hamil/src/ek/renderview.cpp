#include <ek/renderview.h>
#include <ek/renderer.h>
#include <ek/renderobject.h>
#include <ek/constbuffer.h>

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
#include <optional>

namespace ek {

#define USE_MSM

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

  mat4 light_vp;

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

  // Packed object material properties
  //   (uint(material_id), metalness, roughness, 0.0f)
  vec4 material_id__metalness__roughness__0;

  vec4 pad_;
};

struct ObjectShadowConstants {
  mat4 model;
};
#pragma pack(pop)

class RenderViewData {
public:
  // Make SURE to unmap this before rendering
  std::optional<gx::BufferView> object_ubo_view = std::nullopt;

  std::vector<u32> samplers;
};

RenderView::RenderView(ViewType type) :
  m_type(type), m_render((RenderType)~0u),
  m_viewport(0, 0, 0, 0), m_samples(0),
  m_view(mat4::identity()), m_projection(mat4::identity()),
  m_data(new RenderViewData),
  m_renderer(nullptr),
  m_mempool(nullptr),
  m_const_bufs(NumConstantBufferBindings, nullptr),
  m_renderpass_id(gx::ResourcePool::Invalid),
  m_objects_rover(nullptr, 1)
{
  m_ubo_alignment = pow2_round((uint)gx::info().minUniformBindAlignment());
  m_constant_block_sz = (u32)gx::info().maxUniformBlockSize();

  std::fill(m_num_objects_per_type.begin(), m_num_objects_per_type.end(), 0);
}

RenderView::~RenderView()
{
  if(deref()) return;

  for(auto rt : m_rts) renderer().releaseRenderTarget(*rt);
  for(auto buf : m_const_bufs) renderer().releaseConstantBuffer(*buf);
  for(auto sampler : m_data->samplers) pool().release<gx::Sampler>(sampler);

  if(m_renderpass_id != gx::ResourcePool::Invalid)  pool().release<gx::RenderPass>(m_renderpass_id);

  delete m_data;
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
  std::vector<RenderObject>& objects)
{
  // Used by internal methods
  m_renderer = &renderer;

  auto ltc = m_renderer->queryLUT(RenderLUT::LTC_Coeffs);

  // Internal to this RenderView
  m_mempool = new gx::MemoryPool(MempoolInitialAlloc);

  auto scene_constants = generateSceneConstants();

  m_renderpass_id = createRenderPass();
  auto& renderpass = getRenderpass();

  initConstantBuffers(objects.size());

  initLuts();

  auto cmd = gx::CommandBuffer::begin()
    .bindResourcePool(&pool())
    .bindMemoryPool(m_mempool)
    .renderpass(m_renderpass_id)
    .bufferUpload(constantBufferId(SceneConstantsBinding), scene_constants.h, scene_constants.sz);

  std::sort(objects.begin(), objects.end(), [](const RenderObject& a, const RenderObject& b) {
    return a.type() < b.type();
  });

  auto render_one = RenderFns[m_type][m_render];

  size_t num_meshes = m_num_objects_per_type.at(RenderObject::Mesh);
  for(size_t i = 0; i < num_meshes; i++) {
    const auto& ro = objects[i];

    (this->*render_one)(ro, cmd);
    advanceConstantBlockBinding(cmd);
  }

  // Unmap the ObjectConstants UniformBuffer
  m_data->object_ubo_view = std::nullopt;

#if defined USE_MSM
  if(m_type == ShadowView) {
    auto shadow_map = presentRenderTarget().textureId(RenderTarget::Moments);

    cmd.generateMipmaps(shadow_map);
  }
#endif

  return cmd.end();
}

const RenderTarget& RenderView::presentRenderTarget() const
{
  return *m_rts.at(0);
}

RenderView& RenderView::addInput(const RenderView *input)
{
  m_inputs.push_back(input);

  return *this;
}

std::string RenderView::labelPrefix() const
{
#if !defined(NDEBUG)
  switch(m_type) {
  case CameraView: return "CameraView";
  case LightView:  return "LightView";
  case ShadowView: return "ShadowView";
  }
#endif

  return "";
}

gx::ResourcePool& RenderView::pool()
{
  return renderer().pool();
}

gx::Pipeline RenderView::createPipeline()
{
  auto pipeline = gx::Pipeline()
    .viewport(m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w)
    .depthTest(gx::LessEqual)
    .cull(gx::Pipeline::Back)
    .noBlend();

  return pipeline;
}

u32 RenderView::createFramebuffer()
{
  RenderTargetConfig config;

  switch(m_render) {
  case DepthOnly:
    switch(m_type) {
    case CameraView: config = RenderTargetConfig::depth_prepass(m_samples); break;
    case ShadowView: config = RenderTargetConfig::moment_shadow_map(m_samples); break;
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

  const auto& rt = renderer().queryRenderTarget(config);
  m_rts.emplace_back(&rt);

  return rt.framebufferId();
}

u32 RenderView::createRenderPass()
{
  switch(m_type) {
  case CameraView: return createForwardRenderPass();
  case ShadowView: return createShadowRenderPass();
  }

  return ~0u;
}

u32 RenderView::createForwardRenderPass()
{
  auto id = pool().create<gx::RenderPass>();

  auto framebuffer_id = createFramebuffer();
  auto pipeline = createPipeline();

  auto& pass = pool().get<gx::RenderPass>(id);

  pass
    .framebuffer(framebuffer_id)
    .pipeline(pipeline)
    .clearOp(gx::RenderPass::ClearColorDepth);

  // Temporary!

  if(m_inputs.empty()) return id; // ShadowMap not provided

  const auto& shadow_rt = m_inputs.at(0)->presentRenderTarget();

#if defined(USE_MSM)
  auto shadow_map = shadow_rt.textureId(RenderTarget::Moments);
  auto shadow_map_sampler = pool().create<gx::Sampler>(
    "sShadowMapMoments",
    gx::Sampler::edgeclamp2d_mipmap()
  );
#else
  auto shadow_map = shadow_rt.textureId(RenderTarget::Depth);
  auto shadow_map_sampler = pool().create<gx::Sampler>(
    "sShadowMapDepth",
    gx::Sampler::edgeclamp2d_linear()
      .compareRef(gx::Less)
  );
#endif

  m_data->samplers.push_back(shadow_map_sampler);

  // Bind the ShadowMap to a tex unit
  pass
    .texture(ShadowMapTexImageUnit, shadow_map, shadow_map_sampler);

  return id;
}

u32 RenderView::createShadowRenderPass()
{
  auto id = pool().create<gx::RenderPass>();

  auto framebuffer_id = createFramebuffer();
  auto pipeline = createPipeline();

  auto& pass = pool().get<gx::RenderPass>(id);

  pass
    .framebuffer(framebuffer_id)
    .pipeline(pipeline
      .clear({ 0.0f, 0.0f, 0.0f, 0.0f }, 1.0f))
    .clearOp(gx::RenderPass::ClearColorDepth);

  return id;
}

u32 RenderView::constantBufferId(u32 which)
{
  return m_const_bufs.at(which)->id();
}

gx::UniformBuffer& RenderView::constantBuffer(u32 which)
{
  return m_const_bufs.at(which)->get(pool());
}

u32 RenderView::constantBlockSizeAlign(u32 sz)
{
  return pow2_align(sz, m_ubo_alignment);
}

gx::RenderPass& RenderView::getRenderpass()
{
  return pool().get<gx::RenderPass>(m_renderpass_id);
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

  m_const_bufs[SceneConstantsBinding] = &renderer().queryConstantBuffer(
    constantBlockSizeAlign(SceneConstantsSize),
    labelPrefix() + "SceneConstants"
  );
  m_const_bufs[ObjectConstantsBinding] = &renderer().queryConstantBuffer(
    constantBlockSizeAlign((u32)ObjectConstantBufferSize),
    labelPrefix() + "ObjectConstants"
  );

  m_num_objects_per_block = std::min(m_constant_block_sz / ObjectConstantsSize, 256u);

  m_data->object_ubo_view.emplace(
    constantBuffer(ObjectConstantsBinding).map(gx::Buffer::Write, gx::Buffer::MapInvalidate)
  );

  m_objects = m_data->object_ubo_view->get<ObjectConstants>();
  m_objects_rover = StridePtr<ObjectConstants>(m_objects, ObjectConstantsSize);
  m_objects_end = (ObjectConstants *)((byte *)m_objects + ObjectConstantBufferSize);

  getRenderpass()
    .uniformBufferRange(SceneConstantsBinding, constantBufferId(SceneConstantsBinding),
       0, SceneConstantsSize)
    .uniformBufferRange(ObjectConstantsBinding, constantBufferId(ObjectConstantsBinding),
      0, m_constant_block_sz);
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
    .uniformBufferRange(ObjectConstantsBinding, constantBufferId(ObjectConstantsBinding),
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

  if(m_type == CameraView && !m_inputs.empty()) {
    auto light_view = m_inputs.at(0);
    mat4 light_vp = light_view->m_projection * light_view->m_view;

    scene->light_vp = light_vp;
  }

  // TODO: pass these in as a parameter...
  vec4 ambient_basis[6] = {
    vec4(0.0f, 0.25f, 0.25f, 1.0f),  vec4(0.5f, 0.5f, 0.0f, 1.0f),
    vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.7f, 0.7f, 0.7f, 1.0f),
    vec4(0.1f, 0.1f, 0.1f, 1.0f),    vec4(0.1f, 0.1f, 0.1f, 1.0f),
  };
  memcpy(scene->ambient_basis, ambient_basis, sizeof(SceneConstants::ambient_basis));

  memset(scene->light_types, 0, sizeof(SceneConstants::light_types));

  scene->num_lights.x = 3;
  scene->lights[0] = {
    m_view * vec4(0.0f, 6.0f, 0.0f, 1.0f),
    vec3{ 20.0f, 20.0f, 20.0f }
  };
  scene->lights[1] = {
    m_view * vec4(-10.0f, 6.0f, -10.0f, 1.0f),
    vec3{ 20.0f, 20.0f, 0.0f }
  };
  scene->lights[2] = {
    m_view * vec4(20.0f, 6.0f, 0.0f, 1.0f),
    vec3{ 0.0f, 20.0f, 20.0f }
  };

  memcpy(scene->ambient_basis, ambient_basis, sizeof(SceneConstants::ambient_basis));

  return constants;
}

u32 RenderView::writeConstants(const RenderObject& ro)
{
  assert(m_objects_rover.get() < m_objects_end && "Wrote too many ObjectConstants!");

  // Move the rover forward
  ObjectConstants& object = *m_objects_rover++;

  auto& mesh = ro.mesh();

  const auto& model_matrix = mesh.model;
  const auto& material = mesh.material();

  object.model   = model_matrix;
  object.normal  = model_matrix.inverse().transpose();
  object.texture = mat4::identity();

  uint material_id = 0;
  switch(material.diff_type) {
  case hm::Material::DiffuseConstant: material_id = ConstantColor; break;
  case hm::Material::DiffuseTexture:  material_id = Textured; break;
  case hm::Material::Other:           material_id = ProceduralColor; break;

  default: material_id = Unshaded; break;
  }

  object.diff_color = material.diff_color;
  object.ior = vec4(material.ior, 1.0f);

  object.material_id__metalness__roughness__0 = vec4(
    (float)material_id, material.metalness, material.roughness, 0.0f
  );

  size_t buffer_off = &object - m_objects;

  // The buffer is divided into blocks where one block
  //   has the max possible size bindable at once by the GPU,
  //   thus we need to extract the offset of the object in the
  //   CURRENTLY BOUND block
  u32 block_off = (u32)(buffer_off % m_num_objects_per_block);

  return block_off;
}

void RenderView::initLuts()
{
  auto blur_lut_id = renderer().queryLUT(RenderLUT::GaussianKernel, GaussianBlurRadius);
  auto blur_sampler_id = m_data->samplers.emplace_back(
    pool().create<gx::Sampler>("sGaussianKernel", gx::Sampler::borderclamp1d())
  );

  getRenderpass()
    .texture(BlurKernelTexImageUnit, blur_lut_id, blur_sampler_id);
}

// TODO
void RenderView::forwardCameraRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants_offset = writeConstants(ro);

  auto program_id = renderer().queryProgram(*this, ro);
  auto& program = pool().get<gx::Program>(program_id);  // R.shader.shaders.forward

  if(m_init_programs.find(program_id) == m_init_programs.end()) {
    program.use()
      .uniformBlockBinding("SceneConstantsBlock", SceneConstantsBinding)
      .uniformBlockBinding("ObjectConstantsBlock", ObjectConstantsBinding)

      .uniformSampler(U.forward.uDiffuseTex, DiffuseTexImageUnit)
      .uniformSampler(U.forward.uShadowMap, ShadowMapTexImageUnit)
      .uniformSampler(U.forward.uGaussianKernel, BlurKernelTexImageUnit);

    m_init_programs.insert(program_id);
  }

  cmd
    .program(program_id)
    .uniformInt(U.forward.uObjectConstantsOffset, constants_offset);

  auto& renderpass = getRenderpass();

  const auto& material = ro.mesh().material();

  // TODO!
  //   - Batch RenderObject by Diffuse texture
  if(material.diff_type == hm::Material::DiffuseTexture) {
    auto next_subpass = renderpass.nextSubpassId();
    auto subpass = gx::RenderPass::Subpass()
      .texture(DiffuseTexImageUnit, material.diff_tex.id, material.diff_tex.sampler_id);

    renderpass.subpass(subpass);
    cmd.subpass(next_subpass);
  }

  emitDraw(ro.mesh(), cmd);
}

void RenderView::shadowRenderOne(const RenderObject& ro, gx::CommandBuffer& cmd)
{
  auto constants_offset = writeConstants(ro);

  auto program_id = renderer().queryProgram(*this, ro);
  auto& program = pool().get<gx::Program>(program_id);  // R.shader.shaders.rendermsm

  if(m_init_programs.find(program_id) == m_init_programs.end()) {
    program.use()
      .uniformBlockBinding("SceneConstantsBlock", SceneConstantsBinding)
      .uniformBlockBinding("ObjectConstantsBlock", ObjectConstantsBinding);

    m_init_programs.insert(program_id);
  }

  cmd
    .program(program_id)
    .uniformInt(U.rendermsm.uObjectConstantsOffset, constants_offset);

  emitDraw(ro.mesh(), cmd);
}

void RenderView::emitDraw(const RenderMesh& ro, gx::CommandBuffer& cmd)
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