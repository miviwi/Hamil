#include <ek/renderobject.h>

namespace ek {

RenderObject::RenderObject(hm::Entity e) :
  m_entity(e)
{
}

RenderObject& RenderObject::model(const mat4& m)
{
  m_model = m;

  return *this;
}

const mat4& RenderObject::model() const
{
  return m_model;
}

RenderObject& RenderObject::mesh(hm::ComponentRef<hm::Mesh> mesh)
{
  m_mesh = mesh;

  return *this;
}

const hm::Mesh& RenderObject::mesh() const
{
  return m_mesh();
}

RenderObject& RenderObject::material(hm::ComponentRef<hm::Material> material)
{
  m_material = material;

  return *this;
}

const hm::Material& RenderObject::material() const
{
  return m_material();
}

}