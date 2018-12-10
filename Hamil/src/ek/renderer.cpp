#include <ek/renderer.h>

#include <math/util.h>
#include <math/brdf.h>
#include <math/ltc.h>
#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/gameobject.h>
#include <hm/components/transform.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>
#include <gx/resourcepool.h>
#include <gx/program.h>
#include <res/res.h>
#include <res/handle.h>
#include <res/shader.h>

#include <resources.h>
#include <uniforms.h>

#include <cassert>
#include <algorithm>

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
  case LTC_Coeffs:     generateLTC(pool); break;

  default: assert(0);  // unreachable
  }
}

void RenderLUT::generateGaussian(gx::ResourcePool& pool)
{
  auto kernel = gaussian_kernel(param);

  tex_id = pool.createTexture<gx::Texture1D>(gx::r32f);
  auto blur_kernel = pool.getTexture(tex_id);
  blur_kernel()
    .init(kernel.data(), 0, (unsigned)kernel.size(), gx::r, gx::f32);
}

void RenderLUT::generateLTC(gx::ResourcePool& pool)
{
  return;
  auto brdf_ggx = brdf::BRDF_GGX();
  auto ltc_coeffs = ltc::LTC_CoeffsTable();

  static constexpr auto tex_size =  ltc::LTC_CoeffsTable::TableSize;

  ltc_coeffs.fit(brdf_ggx);

  tex_id = pool.createTexture<gx::Texture2DArray>(gx::rgba32f);
  auto ltc = pool.getTexture(tex_id);

  ltc().init(tex_size.s, tex_size.t, 2);
  ltc().upload(ltc_coeffs.coeffs1().data(), 0, 0, 0, 0, tex_size.s, tex_size.t, 1,
    gx::rgba, gx::f32);
  ltc().upload(ltc_coeffs.coeffs1().data(), 0, 0, 0, 1, tex_size.s, tex_size.t, 1,
    gx::rgba, gx::f32);
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

  auto& lut = m_luts.emplace_back();
  lut.type = RenderLUT::LTC_Coeffs;

  lut.generate(pool());
}

Renderer& Renderer::cachePrograms()
{
  if(!m_programs.empty()) return *this; // Already cached

  // Load dependencies
  res::load(R.shader.shaders.util);
  res::load(R.shader.shaders.ubo);
  res::load(R.shader.shaders.blur);
  res::load(R.shader.shaders.msm);

  res::load(R.shader.shaders.ids);

  res::Handle<res::Shader> f_forward = R.shader.shaders.forward;
  res::Handle<res::Shader> f_rendermsm = R.shader.shaders.rendermsm;

  auto forward = pool().create<gx::Program>(gx::make_program(
    f_forward->source(res::Shader::Vertex), f_forward->source(res::Shader::Fragment), U.forward));
  auto rendermsm = pool().create<gx::Program>(gx::make_program(
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

const ConstantBuffer& Renderer::queryConstantBuffer(size_t sz)
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

  auto id = pool().createBuffer<gx::UniformBuffer>(gx::Buffer::Dynamic);
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

gx::ResourcePool& Renderer::pool()
{
  return m_data->pool;
}

Renderer::ObjectVector Renderer::doExtractForView(hm::Entity scene, RenderView& view)
{
  ObjectVector objects;
  auto frustum = view.constructFrustum();

  // Make the Components immutable
  hm::components().lock();

  auto transform_matrix = scene.component<hm::Transform>().get().matrix();
  scene.gameObject().foreachChild([&](hm::Entity e) {
    extractOne(objects, frustum, e, transform_matrix);
  });

  // Done reading components
  hm::components().unlock();

  return objects;
}

void Renderer::extractOne(ObjectVector& objects,
  const frustum3& frustum, hm::Entity e, const mat4& parent)
{
  auto transform = e.component<hm::Transform>();

  auto model_matrix = parent * transform().matrix();
  auto aabb = transform().aabb;

  // Cull the object and it's children
  if(!frustum.aabbInside(aabb)) return;

  auto mesh = e.component<hm::Mesh>();
  auto material = e.component<hm::Material>();

  // Extract the object
  objects.emplace_back(e)
    .model(model_matrix)
    .mesh(mesh)
    .material(material);

  // ...and it's children
  e.gameObject().foreachChild([&](hm::Entity child) {
    extractOne(objects, frustum, child, model_matrix);
  });
}

}
