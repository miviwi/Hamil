#pragma once

#include <ek/euklid.h>

#include <math/geometry.h>
#include <hm/entity.h>
#include <hm/component.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>
#include <hm/components/light.h>

#include <variant>

namespace ek {

class RenderMesh {
public:
  RenderMesh() { }

  // Model matrix extracted at the beginning of rendering
  mat4 model;

  AABB aabb;

  // Cached at the start of rendering
  hm::ComponentRef<hm::Mesh> mesh = nullptr;
  // Cached at the start of rendering
  hm::ComponentRef<hm::Material> material = nullptr;
};

class RenderLight {
public:
  // Position/direction extracted at the beginning of rendering
  vec3 position;

  // Cached at the start of rendering
  hm::ComponentRef<hm::Light> light = nullptr;
};

class RenderObject {
public:
  enum Type {
    // Used as 'm_data' std::variant indices

    Light = 1, Mesh = 2,
    NumTypes
  };

  RenderObject(Type t, hm::Entity e);

  Type type() const;

  RenderMesh& mesh();
  const RenderMesh& mesh() const;

  RenderLight& light();
  const RenderLight& light() const;

private:
  // The Entity which this RenderObject represents
  hm::Entity m_entity;

  std::variant<std::monostate,
    RenderLight,
    RenderMesh> m_data;
};

}