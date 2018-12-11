#pragma once

#include <ek/euklid.h>

#include <math/geometry.h>
#include <hm/entity.h>
#include <hm/component.h>
#include <hm/components/mesh.h>
#include <hm/components/material.h>

#include <variant>

namespace ek {

class RenderMesh {
public:
  RenderMesh() { }

  // Model matrix extracted at the beginning of rendering
  mat4 model;

  // Cached at the start of rendering
  hm::ComponentRef<hm::Mesh> mesh = nullptr;
  // Cached at the start of rendering
  hm::ComponentRef<hm::Material> material = nullptr;
};

class RenderLight {
public:
  enum Type {
    Directional, Spot,
    Line, Rectangle, Quad, Sphere, Disk,
    ImageBasedLight,
  };

  RenderLight() :
    directional({ vec3::zero() })  // Need to have this...
  { }

  Type type;

  float radius;
  vec3 color;

  union {
    struct {
      vec3 direction;
    } directional;

    struct {
      vec3 origin;
      vec3 direction;
      float angle;
    } spot;

    struct {
      vec3 v1, v2;
    } line;

    struct {
      vec3 center;
      float radius;
    } sphere;
  };
};


class RenderObject {
public:
  enum Type {
    // Used as 'm_data' std::variant indices

    Mesh = 1, Light = 2,
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
    RenderMesh,
    RenderLight> m_data;
};

}