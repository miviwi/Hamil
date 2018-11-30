#pragma once

#include <ek/euklid.h>

#include <math/geometry.h>
#include <hm/entity.h>
#include <hm/component.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>

namespace ek {

class RenderObject {
public:
  RenderObject(hm::Entity e);

  RenderObject& model(const mat4& m);
  const mat4& model() const;

  RenderObject& mesh(hm::ComponentRef<hm::Mesh> mesh);
  const hm::Mesh& mesh() const;

  RenderObject& material(hm::ComponentRef<hm::Material> material);
  const hm::Material& material() const;

private:
  // Model matrix extracted at the beginning of rendering
  mat4 m_model;

  // The Entity which this RenderObject represents
  hm::Entity m_entity;

  // Cached at the start of rendering
  hm::ComponentRef<hm::Mesh> m_mesh = nullptr;
  // Cached at the start of rendering
  hm::ComponentRef<hm::Material> m_material = nullptr;
};

}