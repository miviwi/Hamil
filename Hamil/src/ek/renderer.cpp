#include <ek/renderer.h>

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

Renderer::Renderer()
{
  m_rts.reserve(InitialRenderTargets);
}

Renderer::ExtractObjectsJob Renderer::extractForView(hm::Entity scene, RenderView& view)
{
  return ExtractObjectsJob(new sched::Job<ObjectVector, hm::Entity, RenderView *>(
    sched::create_job([this](hm::Entity scene, RenderView *view) -> ObjectVector {
      return doExtractForView(scene, *view);
    }, scene, &view)
  ));
}

const RenderTarget& Renderer::queryRenderTarget(const RenderTargetConfig& config, gx::ResourcePool& pool)
{
  // Initially we only need to read from the vector
  m_rts_lock.acquireShared();

  auto it = m_rts.begin();
  while(it != m_rts.end()) {
    auto next = std::find_if(m_rts.begin(), m_rts.end(), [&](const RenderTarget& rt) {
      return rt.config() == config;
    });

    // Check if the RenderTarget isn't already in use
    if(it->lock()) {
      // Release the lock
      m_rts_lock.releaseShared();

      return *it;
    }

    next = it;
  }

  // No free RenderTarget available - need to write to the vector
  m_rts_lock.releaseShared();
  m_rts_lock.acquireExclusive();

  auto& result = m_rts.emplace_back(RenderTarget::from_config(config, pool));

  // Done Writing
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
u32 Renderer::queryProgram(const RenderObject& ro, gx::ResourcePool& pool)
{
  // Temporary!
  if(m_programs.empty()) {
    m_programs_lock.acquireExclusive();

    res::load(&R.shader.shaders.program, 1);   // Make sure it's loaded
    res::Handle<res::Shader> r_program = R.shader.shaders.program;

    auto id = pool.create<gx::Program>(gx::make_program(
      r_program->source(res::Shader::Vertex), r_program->source(res::Shader::Fragment), U.program));
    m_programs.push_back(id);

    m_programs_lock.releaseExclusive();
  }

  m_programs_lock.acquireShared();

  auto program = m_programs.front(); // TODO!
  m_programs_lock.releaseShared();

  return program;
}

Renderer::ObjectVector Renderer::doExtractForView(hm::Entity scene, RenderView& view)
{
  ObjectVector objects;
  auto frustum = view.constructFrustum();
  auto transform_matrix = scene.component<hm::Transform>().get().matrix();
  scene.gameObject().foreachChild([&](hm::Entity e) {
    extractOne(objects, frustum, e, transform_matrix);
  });

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
