#include <ek/renderobject.h>

namespace ek {

RenderObject::RenderObject(Type t, hm::Entity e) :
  m_entity(e),
  m_data(std::monostate())
{
  switch(t) {
  case Mesh:  m_data.emplace<Mesh>(); break;
  case Light: m_data.emplace<Light>(); break;
  }
}

RenderObject::Type RenderObject::type() const
{
  return (Type)m_data.index();
}

RenderMesh& RenderObject::mesh()
{
  return std::get<Mesh>(m_data);
}

const RenderMesh& RenderObject::mesh() const
{
  return std::get<Mesh>(m_data);
}

RenderLight& RenderObject::light()
{
  return std::get<Light>(m_data);
}

const RenderLight& RenderObject::light() const
{
  return std::get<Light>(m_data);
}


}