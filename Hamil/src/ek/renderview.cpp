#include <ek/renderview.h>
#include <ek/renderer.h>
#include <ek/renderobject.h>
#include <ek/constbuffer.h>
#include <ek/visibility.h>
#include <ek/visobject.h>
#include <ek/occlusion.h>

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
#include <gx/fence.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>
#include <hm/components/light.h>

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

enum {
  MaxForwardPassLights = 8,
};

enum MaterialId : u32 {
  Unshaded,
  ConstantColor,
  ProceduralColor,
  Textured,
};

#pragma pack(push, 1)
struct alignas(16) LightConstants {
  vec4 v1;
  vec4 v2;
  vec4 v3;
  vec4 v4;

  // SphereLight:
  //   v1 = vec4(position.xyz, radius)
  //   v2 = vec4(color.rgb, sphere_radius)

  // LineLight:
  //   v1 = vec4(p1.xyz, 1.0)
  //   v2 = vec4(p2.xyz, line_radius)
  //   v3 = vec4(color.rgb, 1.0)
};

struct alignas(16) SceneConstants {
  mat4 view;
  mat4 projection;

  // projection * view
  mat4 viewprojection;

  mat4 light_vp;

  vec4 /* vec3 */ ambient_basis[6];

  // Driver issues force this to be an ivec4
  ivec4 /* int */ num_lights;
  // - Each vector stores 4 adjacent LightTypes which
  //   correspond to the lights[] array
  // - Packed to save memory used for alignment padding
  ivec4 light_types[MaxForwardPassLights / 4];
  LightConstants lights[MaxForwardPassLights];
};

struct alignas(16) ObjectConstants {
  mat4 model;
  mat4 normal;
  mat4 texture;

  vec4 /* vec3 */ diff_color;
  vec4 /* vec3 */ ior;

  // Packed object material properties
  //   (intBitsToFloat(material_id), metalness, roughness, 0.0f)
  vec4 materialid_metalness_roughness_0;

  vec4 pad_;
};

struct alignas(16) ObjectShadowConstants {
  mat4 model;
};
#pragma pack(pop)

class RenderViewData {
public:
  // Make SURE to unmap this before calling CommandBuffer::execute()!
  std::optional<gx::BufferView> object_ubo_view = std::nullopt;

  // Pointer to MemoryPool-owned data
  SceneConstants *scene = nullptr;

  // Fence used to guard shared resources given out by Renderer
  //   - sync() is called on this fence after all drawing
  //     commands for this view are issiued
  gx::ResourcePool::Id fence = gx::ResourcePool::Invalid;

  // Requires late-contruction
  std::optional<ViewVisibility> vis = std::nullopt;
};

RenderView::RenderView(ViewType type) :
  m_type(type), m_render((RenderType)~0u),
  m_viewport(0, 0, 0, 0), m_samples(0),
  m_view(mat4::identity()), m_projection(mat4::identity()),
  m_data(new RenderViewData),
  m_renderer(nullptr),
  m_const_bufs(NumConstantBufferBindings, nullptr),
  m_renderpass_id(gx::ResourcePool::Invalid),
  m_objects_rover(nullptr, 1)
{
  m_ubo_alignment = pow2_round((uint)gx::info().minUniformBindAlignment());
  m_constant_block_sz = (u32)gx::info().maxUniformBlockSize();
}

RenderView::~RenderView()
{
  if(deref()) return;

  for(auto rt : m_rts) renderer().releaseRenderTarget(*rt);
  for(auto buf : m_const_bufs) renderer().releaseConstantBuffer(*buf);
  for(auto mp : m_mempools) renderer().releaseMempool(*mp);
  renderer().doneFence(m_data->fence);

  pool().release<gx::RenderPass>(m_renderpass_id);

  delete m_data;
}

void RenderView::init(Renderer& renderer)
{
  // Used by internal methods
  m_renderer = &renderer;

  // Guards resources (RenderTargets, ConstantBuffers...) used by
  //   this RenderView, signaled right before end() in the
  //   returned gx::CommandBuffer
  m_data->fence = createFence();

  m_mempools.push_back(&m_renderer->queryMempool(4096, m_data->fence));

  auto vis_mempool = m_mempools.emplace_back(&m_renderer->queryMempool(
    OcclusionBuffer::MempoolSize, m_data->fence
  ));

  m_data->vis.emplace(*vis_mempool);
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

const mat4& RenderView::view() const
{
  return m_view;
}

RenderView& RenderView::projection(const mat4& p)
{
  m_projection = p;

  return *this;
}

const mat4& RenderView::projection() const
{
  return m_projection;
}

vec3 RenderView::eyePosition() const
{
  return m_view.translation();
}

bool RenderView::wantsLights() const
{
  return m_type == CameraView;
}

bool RenderView::wantsOcclusionCulling() const
{
  return m_type == CameraView;
}

frustum3 RenderView::constructFrustum()
{
  return frustum3(m_view, m_projection);
}

ViewVisibility& RenderView::visibility()
{
  return *m_data->vis;
}

RenderView::RenderJob RenderView::render()
{
  // Because createFramebuffer() will be called by this method
  //   it must be put here (not inside the job) because OpenGL
  //   dissallows sharing Framebuffer ids between threads (contexts)
  m_renderpass_id = createRenderPass();

  return RenderJob(new sched::Job<gx::CommandBuffer, std::vector<RenderObject> *>(
    sched::create_job([this](std::vector<RenderObject> *objects) -> gx::CommandBuffer {
      return doRender(*objects);
    })
  ));
}

const RenderView::RenderFn RenderView::RenderFns[NumViewTypes][NumRenderTypes] = {
  { nullptr, nullptr, nullptr },   // Invalid
  { nullptr, &RenderView::forwardCameraRenderOne, nullptr },   // CameraView
  { nullptr, nullptr, nullptr },   // LightView
  { &RenderView::shadowRenderOne, nullptr, nullptr },   // ShadowView
};

gx::CommandBuffer RenderView::doRender(std::vector<RenderObject>& objects)
{
  [[maybe_unused]] auto ltc = m_renderer->queryLUT(RenderLUT::LTCCoeffs);

  [[maybe_unused]] auto& renderpass = getRenderpass();

  // The integer values of RenderObject::Type are arranged in such a way that after
  //   sorting them the RenderLights will come first, which means that after skipping
  //   them all that remains is the RenderMeshes
  std::sort(objects.begin(), objects.end(), [](const RenderObject& a, const RenderObject& b) {
    return a.type() < b.type();   // Sort the RenderObjects according to their type()
  });

  initConstantBuffers(objects.size());  // Allocate the UniformBuffers
  initLuts();

  // Fill in m_data->scene and return a (gx::MemoryPool::Handle, size)
  auto scene_constants = generateSceneConstants();

  // Number of processed lights == Offset of the RenderMeshes
  auto meshes_off = processLights(objects);

  // Draw the meshes front to back
  auto eye = eyePosition();
  std::sort(objects.begin()+meshes_off, objects.end(), [=](const RenderObject& a, const RenderObject& b) {
    AABB a_aabb = a.mesh().aabb;
    AABB b_aabb = b.mesh().aabb;

    vec3 a_pos = (a_aabb.min + a_aabb.max) * 0.5f;
    vec3 b_pos = (b_aabb.min + b_aabb.max) * 0.5f;

    return eye.distance2(a_pos) > eye.distance2(b_pos);
  });

  auto cmd = gx::CommandBuffer::begin()
    .bindResourcePool(&pool())
    .bindMemoryPool(m_mempools.front()->ptr())
    .renderpass(m_renderpass_id)
    .bufferUpload(constantBufferId(SceneConstantsBinding), scene_constants.h, scene_constants.sz);

  auto& vis = visibility();
#if !defined(NDEBUG)
  int num_culled = 0;
  int num_full_tests = 0;
#endif
  auto render_one = RenderFns[m_type][m_render];
  for(size_t i = meshes_off; i < objects.size(); i++) {
    const auto& ro = objects[i];

    auto vis_object = (VisibilityObject *)ro.mesh().vis().visObject();

    // Run occlusion query
    vis.occlusionQuery(vis_object);

    unsigned meshes_culled = 0;   // Number of meshes culled which are
                                  //   owned by this RenderObject
    vis_object->foreachMesh([&](VisibilityMesh& mesh) {
#if !defined(NDEBUG)
      if(mesh.vis_flags & VisibilityMesh::LateOut) num_full_tests++;
#endif
      // Mesh wasn't culled
      if(mesh.visible != VisibilityMesh::Invisible) return;

      meshes_culled++;
    });

#if !defined(NDEBUG)
    num_culled += meshes_culled;
#endif

    if(meshes_culled == vis_object->numMeshes()) continue;

    (this->*render_one)(ro, cmd);
    advanceConstantBlockBinding(cmd);
  }

#if !defined(NDEBUG)
  //printf("Culled %3d meshes (%3d full tests performed)\n",
  //  num_culled, num_full_tests);
#endif

  // Unmap the ObjectConstants UniformBuffer
  m_data->object_ubo_view = std::nullopt;

#if defined(USE_MSM)
  if(m_type == ShadowView) {
    auto shadow_map = presentRenderTarget().textureId(RenderTarget::Moments);

    cmd.generateMipmaps(shadow_map);
  }
#endif

  cmd.fenceSync(m_data->fence);

  return cmd.end();
}

// TODO
const RenderTarget& RenderView::presentRenderTarget() const
{
  return *m_rts.at(0);
}

RenderView& RenderView::addInput(const RenderView *input)
{
  m_inputs.push_back(input);

  return *this;
}

size_t RenderView::numEmmittedDrawcalls() const
{
  return m_num_drawcalls;
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

  const auto& rt = renderer().queryRenderTarget(config, m_data->fence);
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
  auto shadow_map_sampler = renderer().querySampler(MSMTrilinearSampler);
#else
  auto shadow_map = shadow_rt.textureId(RenderTarget::Depth);
  auto shadow_map_sampler = renderer().querySampler(PCFShadowMapSampler);
#endif

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

u32 RenderView::createFence()
{
  return renderer().queryFence("f" + labelPrefix());
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
    constantBlockSizeAlign(SceneConstantsSize), m_data->fence,
    labelPrefix() + "SceneConstants"
  );
  m_const_bufs[ObjectConstantsBinding] = &renderer().queryConstantBuffer(
    constantBlockSizeAlign((u32)ObjectConstantBufferSize), m_data->fence,
    labelPrefix() + "ObjectConstants"
  );

  m_num_objects_per_block = std::min(m_constant_block_sz / ObjectConstantsSize, 256u);

  m_data->object_ubo_view.emplace(
    constantBuffer(ObjectConstantsBinding).map(gx::Buffer::Write,
      // ConstantBuffers returned by renderer().queryConstantBuffer() are
      //   guarded by a gx::Fence so they're guaranteed to NOT be in-use
      gx::Buffer::MapInvalidate | gx::Buffer::MapUnsynchronized)
  );

  m_objects = m_data->object_ubo_view->get<ObjectConstants>();
  m_objects_rover = StridePtr<ObjectConstants>(m_objects, ObjectConstantsSize);
  m_objects_end = (ObjectConstants *)((byte *)m_objects + ObjectConstantBufferSize);

  getRenderpass()
    .uniformBuffersRange({
      { SceneConstantsBinding,  { constantBufferId(SceneConstantsBinding), 0, SceneConstantsSize } },
      { ObjectConstantsBinding, { constantBufferId(ObjectConstantsBinding), 0, m_constant_block_sz } },
    });
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

  auto& mempool = m_mempools.front()->get();

  constants.sz = constantBlockSizeAlign(sizeof(SceneConstants));
  constants.h  = mempool.alloc(constants.sz);

  m_data->scene = mempool.ptr<SceneConstants>(constants.h);
  SceneConstants *scene = m_data->scene;

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

  scene->num_lights.x = 0;
  memset(scene->light_types, 0, sizeof(SceneConstants::light_types));
  memset(scene->lights, 0, sizeof(SceneConstants::lights));

  return constants;
}

u32 RenderView::writeConstants(const RenderObject& ro)
{
  assert(m_objects_rover.get() < m_objects_end && "Wrote too many ObjectConstants!");

  // Move the rover forward
  ObjectConstants& object = *m_objects_rover++;

  auto& mesh = ro.mesh();

  const auto& model_matrix = mesh.model;
  const auto& material = mesh.mat();

  object.model   = model_matrix;
  object.normal  = model_matrix.inverse().transpose();
  object.texture = mat4::identity();

  union {
    uint material_id;
    float material_idf;   // *(float *)&material_id
  };

  material_id = 0;
  switch(material.diff_type) {
  case hm::Material::DiffuseConstant: material_id = ConstantColor; break;
  case hm::Material::DiffuseTexture:  material_id = Textured; break;
  case hm::Material::Other:           material_id = ProceduralColor; break;

  default: material_id = Unshaded; break;
  }

  object.diff_color = material.diff_color;
  object.ior = vec4(material.ior, 1.0f);

  object.materialid_metalness_roughness_0 = vec4(
    material_idf, material.metalness, material.roughness, 0.0f
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
  auto blur_lut_id = renderer().queryLUT(RenderLUT::GaussianKernel, ShadowBlurRadius);
  auto blur_sampler_id = renderer().querySampler(LUT1DNearestSampler);

  auto ltc_lut_id = renderer().queryLUT(RenderLUT::LTCCoeffs);
  auto ltc_sampler_id = renderer().querySampler(LUT2DLinearSampler);

  getRenderpass()
    .textures({
      { BlurKernelTexImageUnit, { blur_lut_id, blur_sampler_id } },
      { LTCCoeffsTexImageUnit,  { ltc_lut_id, ltc_sampler_id } },
    });
}

// TODO!
size_t RenderView::processLights(const std::vector<RenderObject>& objects)
{
  LightConstants *consts = m_data->scene->lights;
  ivec4 *types = m_data->scene->light_types;

  // The number of lights is stored as a uvec4
  //   for proper alignment
  int& num_lights = m_data->scene->num_lights[0];

  size_t i;
  for(i = 0; i < objects.size(); i++) {
    const auto& ro = objects[i];
    if(ro.type() != RenderObject::Light) break;

    // Skip lights over the limit
    // TODO: use lights which most contribute to the
    //       scene instead of cutting off arbitrarily
    if(num_lights >= MaxForwardPassLights) continue;

    auto light_type = ro.light().light().type;

    // Types are encoded in uvec4's
    types[num_lights>>2][num_lights&3] = light_type;

    switch(light_type) {
    case hm::Light::Sphere: *consts++ = generateSphereLightConstants(ro); break;
    case hm::Light::Line:   *consts++ = generateLineLightConstants(ro); break;
    }

    num_lights++;
  }

  return i;  // Number of processed lights
}

LightConstants RenderView::generateSphereLightConstants(const RenderObject& ro)
{
  LightConstants consts;
  auto light = ro.light().light;

  // Transform the light's position into view space
  auto center = vec4(ro.light().position, 1.0f);
  center = m_view * center;

  // Pack the light data
  consts.v1 = vec4(center.xyz(), light().radius);
  consts.v2 = vec4(light().color, light().sphere.radius);

  return consts;
}

LightConstants RenderView::generateLineLightConstants(const RenderObject& ro)
{
  LightConstants consts;
  auto light = ro.light().light;

  auto center = ro.light().position;
  auto tangent = light().line.tangent * light().line.length*0.5f;

  auto p1 = vec4(center+tangent, 1.0f);
  auto p2 = vec4(center-tangent, 1.0f);

  p1 = m_view * p1;
  p2 = m_view * p2;

  consts.v1 = p1;
  consts.v2 = vec4(p2.xyz(), light().radius);
  consts.v3 = vec4(light().color, 1.0f);

  return consts;
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
      .uniformSampler(U.forward.uGaussianKernel, BlurKernelTexImageUnit)
      .uniformSampler(U.forward.uLTC_Coeffs, LTCCoeffsTexImageUnit);

    m_init_programs.insert(program_id);
  }

  cmd
    .program(program_id)
    .uniformInt(U.forward.uObjectConstantsOffset, constants_offset);

  auto& renderpass = getRenderpass();

  const auto& material = ro.mesh().mat();

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
  const auto& meshes = ro.mesh().m;

  meshes.foreach([&](const mesh::Mesh& mesh) {
    if(mesh.isIndexed()) {
      if(mesh.base != mesh::Mesh::None && mesh.offset != mesh::Mesh::None) {
        cmd.drawBaseVertex(mesh.getPrimitive(), mesh.vertex_array_id,
          mesh.num, mesh.base, mesh.offset);
      } else if(mesh.offset != mesh::Mesh::None) {
        cmd.drawBaseVertex(mesh.getPrimitive(), mesh.vertex_array_id,
          mesh.num, 0, mesh.offset);
      } else { // Use the shorter command when possible
        cmd.drawIndexed(mesh.getPrimitive(), mesh.vertex_array_id, mesh.num);
      }
    } else {
      cmd.draw(mesh.getPrimitive(), mesh.vertex_array_id, mesh.num);
    }
  });

  m_num_drawcalls += meshes.size();
}

}
