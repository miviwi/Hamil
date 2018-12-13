#include <ek/renderer.h>

#include <util/format.h>
#include <math/util.h>
#include <math/brdf.h>
#include <math/ltc.h>
#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/gameobject.h>
#include <hm/components/transform.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>
#include <hm/components/light.h>
#include <gx/resourcepool.h>
#include <gx/program.h>
#include <res/res.h>
#include <res/handle.h>
#include <res/shader.h>
#include <res/lut.h>

#include <resources.h>
#include <uniforms.h>

#include <cassert>
#include <algorithm>
#include <random>

namespace ek {

RenderLUT::RenderLUT() :
  type(NumTypes), param(-1),
  tex_id(gx::ResourcePool::Invalid)
{
}

void RenderLUT::generate(gx::ResourcePool& pool)
{
  switch(type) {
  case GaussianKernel: generateGaussian(pool); break;
  case LTCCoeffs:     generateLTC(pool); break;
  case HBAONoise:     generateHBAONoise(pool); break;

  default: assert(0);  // unreachable
  }
}

void RenderLUT::generateGaussian(gx::ResourcePool& pool)
{
  auto kernel = gaussian_kernel(param);

  auto kernel_dims = param*2 + 1;
  auto tex_label = util::fmt("t1dGaussianKernel%dx%d", kernel_dims, kernel_dims);

  tex_id = pool.createTexture<gx::Texture1D>(tex_label.data(), gx::r32f);
  auto blur_kernel = pool.getTexture(tex_id);
  blur_kernel()
    .init(kernel.data(), 0, (unsigned)kernel.size(), gx::r, gx::f32);
}

void RenderLUT::generateLTC(gx::ResourcePool& pool)
{
  static constexpr auto TexSize = ltc::LTC_CoeffsTable::TableSize;

  res::Handle<res::LookupTable> r_lut = R.lut.ltc_lut;

  auto coeffs1 = (float *)r_lut->data();
  auto coeffs2 = coeffs1 + TexSize.area();

  std::string tex_label = "t2daLTCCoefficients";

  tex_id = pool.createTexture<gx::Texture2DArray>(tex_label.data(), gx::rgba32f);
  auto ltc = pool.getTexture(tex_id);

  ltc().init(TexSize.s, TexSize.t, 2);
  ltc().upload(coeffs1, /* mip */ 0,
    /* x */ 0, /* y */ 0, /* z */ 0, TexSize.s, TexSize.t, 1,
    gx::rgba, gx::f32);
  ltc().upload(coeffs2, /* mip */ 0,
    /* x */ 0, /* y */ 0, /* z */ 1, TexSize.s, TexSize.t, 1,
    gx::rgba, gx::f32);
}

void RenderLUT::generateHBAONoise(gx::ResourcePool& pool)
{
  static constexpr uvec2 NoiseSize = { 4, 4 };
  const float NumDirections = param > 0 ? (float)param : 8.0f;

  // Generate random rotations and offsets for HBAO kernel

  std::mt19937 rd;
  std::uniform_real_distribution<float> floats(0.0f, 1.0f);
  std::array<vec3, NoiseSize.area()> noise;
  for(auto& sample : noise) {
    float r0 = floats(rd),
      r1 = floats(rd);

    float angle = 2.0f*PIf*r0 / NumDirections;

    // sample = vec3(cos(rotation), sin(rotation), start_offset)
    sample = vec3(cosf(angle), sinf(angle), r1);
  }

  auto tex_label = util::fmt("t2dHBAONoise%d", (int)NumDirections);

  tex_id = pool.createTexture<gx::Texture2D>(tex_label.data(), gx::rgb32f);
  auto noise_tex = pool.getTexture(tex_id);

  noise_tex()
    .init(noise.data(), 0, NoiseSize.s, NoiseSize.t, gx::rgb, gx::f32);
}

class RendererData {
public:
  enum { InitialResourcePoolAlloc = 2048, };

  RendererData() : pool(InitialResourcePoolAlloc) { }

  gx::ResourcePool pool;
};

Renderer::Renderer() :
  m_data(new RendererData)
{
  m_rts.reserve(InitialRenderTargets);
  m_const_buffers.reserve(InitialConstantBuffers);

  precacheLUTs();
  precacheSamplers();
}

Renderer& Renderer::cachePrograms()
{
  if(!m_programs.empty()) return *this; // Already cached

  // Load dependencies
  res::load(R.shader.shaders.util);
  res::load(R.shader.shaders.ubo);
  res::load(R.shader.shaders.blur);
  res::load(R.shader.shaders.msm);
  res::load(R.shader.shaders.ltc);

  res::load(R.shader.shaders.ids);

  res::Handle<res::Shader> f_forward = R.shader.shaders.forward;
  res::Handle<res::Shader> f_rendermsm = R.shader.shaders.rendermsm;

  auto forward = pool().create<gx::Program>(gx::make_program("pForward",
    f_forward->source(res::Shader::Vertex), f_forward->source(res::Shader::Fragment), U.forward));
  auto rendermsm = pool().create<gx::Program>(gx::make_program("pMSM",
    f_rendermsm->source(res::Shader::Vertex), f_rendermsm->source(res::Shader::Fragment), U.rendermsm));

  // Writing to 'm_programs'
  m_programs_lock.acquireExclusive();

  m_programs.push_back(forward);
  m_programs.push_back(rendermsm);

  m_programs_lock.releaseExclusive();

  return *this;
}

Renderer::ExtractObjectsJob Renderer::extractForView(hm::Entity scene, RenderView& view)
{
  return ExtractObjectsJob(new sched::Job<ObjectVector, hm::Entity, RenderView *>(
    sched::create_job([this](hm::Entity scene, RenderView *view) -> ObjectVector {
      return doExtractForView(scene, *view);
    }, scene, &view)
  ));
}

const RenderTarget& Renderer::queryRenderTarget(const RenderTargetConfig& config)
{
  // Initially we only need to read from the vector
  m_rts_lock.acquireShared();

  auto it = m_rts.begin();
  while(it != m_rts.end()) {
    it = std::find_if(m_rts.begin(), m_rts.end(), [&](const RenderTarget& rt) {
      return rt.config() == config;
    });

    if(it == m_rts.end()) break; // No fitting RenderTarget was created

    // Check if the RenderTarget isn't already in use
    if(it->lock()) {
      m_rts_lock.releaseShared();

      return *it;
    }

    // Keep on searching
    it++;
  }

  // Need to create a new RenderTarget which means
  //   we'll be writing to the vector
  m_rts_lock.releaseShared();
  m_rts_lock.acquireExclusive();

  auto& result = m_rts.emplace_back(RenderTarget::from_config(config, pool()));

  // Done writing
  m_rts_lock.releaseExclusive();

  auto locked = result.lock();
  assert(locked && "Failed to lock() the RenderTarget!");

  return result;
}

void Renderer::releaseRenderTarget(const RenderTarget& rt)
{
  rt.unlock();
}

// TODO: Select the program based on the RenderObject
u32 Renderer::queryProgram(const RenderView& view, const RenderObject& ro)
{
  if(m_programs.empty()) cachePrograms();

  // Reading from 'm_programs'
  m_programs_lock.acquireShared();

  u32 program = ~0u;
  switch(view.m_type) {
  case RenderView::CameraView: program = m_programs.at(0); break;
  case RenderView::ShadowView: program = m_programs.at(1); break;
  }

  m_programs_lock.releaseShared();

  return program;
}

const ConstantBuffer& Renderer::queryConstantBuffer(size_t sz, const std::string& label)
{
  m_const_buffers_lock.acquireShared();

  auto it = m_const_buffers.begin();
  while(it != m_const_buffers.end()) {
    it = std::find_if(it, m_const_buffers.end(), [&](const ConstantBuffer& buf) {
      return buf.size() >= sz;
    });

    if(it == m_const_buffers.end()) break;

    if(it->lock()) {
      m_const_buffers_lock.releaseShared();

      return *it;
    }

    // Keep on searching
    it++;
  }

  m_const_buffers_lock.releaseShared();

  std::string buf_label = "bu";
  if(!label.empty()) {
    buf_label += label;
  } else {  // Use the size as the suffix if one wasn't provided
    buf_label += util::fmt("ConstantBuffer%zu", sz);
  }

  auto id = pool().createBuffer<gx::UniformBuffer>(buf_label.data(), gx::Buffer::Dynamic);
  auto buf = pool().getBuffer(id);

  buf().init(sz, 1);

  m_const_buffers_lock.acquireExclusive();
  auto& const_buf = m_const_buffers.emplace_back(ConstantBuffer(id, sz));
  m_const_buffers_lock.releaseExclusive();

  auto result = const_buf.lock();
  assert(result && "failed to lock() ConstantBuffer!");

  return const_buf;
}

void Renderer::releaseConstantBuffer(const ConstantBuffer& buf)
{
  buf.unlock();
}

u32 Renderer::queryLUT(RenderLUT::Type type, int param)
{
  m_luts_lock.acquireShared();

  auto it = m_luts.begin();
  while(it != m_luts.end()) {
    it = std::find_if(it, m_luts.end(), [&](const RenderLUT& lut) {
      return lut.type == type && lut.param == param;
    });

    // No compatible RenderLUT found...
    if(it == m_luts.end()) break;

    // Found one!
    return it->tex_id;
  }

  m_luts_lock.releaseShared();
  m_luts_lock.acquireExclusive();

  // Generate a new RenderLUT
  auto& lut = m_luts.emplace_back();
  lut.type = type; lut.param = param;

  // No longer need to hold the lock,
  //   so release it here because the
  //   following call can potentially
  //   be time-consuming
  m_luts_lock.releaseExclusive();

  // Generate and upload the texture
  lut.generate(pool());

  return lut.tex_id;
}

u32 Renderer::querySampler(SamplerClass sampler)
{
  m_samplers_lock.acquireShared();

  auto it = m_samplers.find(sampler);
  if(it != m_samplers.end()) {
    m_samplers_lock.releaseShared();

    return it->second;
  }

  // The requested gx::Sampler hasn't been
  //   created yet
  m_samplers_lock.releaseShared();

  u32 id = gx::ResourcePool::Invalid;
  switch(sampler) {
  case MSMTrilinearSampler: id = pool().create<gx::Sampler>(
      "sMSMTrilinear", gx::Sampler::edgeclamp2d_mipmap()
    );
    break;

  case PCFShadowMapSampler: id = pool().create<gx::Sampler>(
      "sPCFShadowMap",
      gx::Sampler::edgeclamp2d_linear()
        .compareRef(gx::Less)
    );
    break;

  case HBAONoiseSampler: id = pool().create<gx::Sampler>(
      "sHBAONoise", gx::Sampler::repeat2d()
    );
    break;

  case LUT1DNearestSampler: id = pool().create<gx::Sampler>(
      "sLUT1DNearest", gx::Sampler::edgeclamp1d()
    );
    break;

  case LUT2DLinearSampler: id = pool().create<gx::Sampler>(
      "sLUT2DBilinear", gx::Sampler::edgeclamp2d_linear()
    );
    break;
  }

  // Write to the std::map
  m_samplers_lock.acquireExclusive();
  m_samplers.emplace(sampler, id);
  m_samplers_lock.releaseExclusive();

  return id;
}

gx::ResourcePool& Renderer::pool()
{
  return m_data->pool;
}

void Renderer::precacheLUTs()
{
  // Load pre-computed LUTs
  res::load(R.lut.ids);

  auto& gauss_lut = m_luts.emplace_back();
  gauss_lut.type = RenderLUT::GaussianKernel;
  gauss_lut.param = RenderView::GaussianBlurRadius;
  gauss_lut.generate(pool());

  //auto& ltc_lut = m_luts.emplace_back();
  //ltc_lut.type = RenderLUT::LTC_Coeffs;
  //ltc_lut.generate(pool());

  auto& hbao_nosie_lut = m_luts.emplace_back();
  hbao_nosie_lut.type = RenderLUT::HBAONoise;
  hbao_nosie_lut.generate(pool());
}

void Renderer::precacheSamplers()
{
  // The following method calls are here for
  //   their side-effects only

  querySampler(HBAONoiseSampler);

  querySampler(LUT1DNearestSampler);
  querySampler(LUT2DLinearSampler);
}

Renderer::ObjectVector Renderer::doExtractForView(hm::Entity scene, RenderView& view)
{
  ObjectVector objects;
  auto frustum = view.constructFrustum();

  // Make the Components immutable
  hm::components().lock();

  auto transform_matrix = scene.component<hm::Transform>().get().matrix();
  scene.gameObject().foreachChild([&](hm::Entity e) {
    extractOne(view, objects, frustum, e, transform_matrix);
  });

  // Done reading components
  hm::components().unlock();

  return objects;
}

void Renderer::extractOne(RenderView& view, ObjectVector& objects,
  const frustum3& frustum, hm::Entity e, const mat4& parent)
{
  auto transform = e.component<hm::Transform>();
  auto model_matrix = parent * transform().matrix();
  auto aabb = transform().aabb;

  // Extract children
  e.gameObject().foreachChild([&](hm::Entity child) {
    extractOne(view, objects, frustum, child, model_matrix);
  });

  // Extract the object
  if(auto mesh = e.component<hm::Mesh>()) {
    // Cull it
    if(!frustum.aabbInside(aabb)) return;

    auto material = e.component<hm::Material>();

    auto& ro = objects.emplace_back(RenderObject::Mesh, e).mesh();

    ro.model    = model_matrix;
    ro.mesh     = mesh;
    ro.material = material;
  } else if(auto light = e.component<hm::Light>()) {
    // Check if this view needs to have RenderLights extracted
    if(view.m_type != RenderView::CameraView) return;

    vec3 position = model_matrix.translation();

    float radius = light().radius;
    float radius2 = radius*radius;
    float distance2 = view.eyePosition().distance2(position);

    // TODO: Better light culling
    if(!frustum.sphereInside(position, radius) &&
      distance2 > radius2) return;

    auto& ro = objects.emplace_back(RenderObject::Light, e).light();

    ro.position = position;
    ro.light = light;
  }
}

}
