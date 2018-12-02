#include <ek/renderer.h>

#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/gameobject.h>
#include <hm/components/transform.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>
#include <gx/resourcepool.h>

#include <cassert>

namespace ek {

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
  RenderTarget k(config);

  auto it = m_rts.find(k);
  if(it != m_rts.end()) {
    if(it->lock()) return *it;
  }

  auto result = m_rts.insert(RenderTarget::from_config(config, pool));
  assert(result.second && "Failed to insert RenderTarget!");

  auto locked = result.first->lock();
  assert(locked && "Failed to lock() the RenderTarget!");

  return *result.first;
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
