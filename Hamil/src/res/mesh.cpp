#include <res/mesh.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <gx/gx.h>

#include <cassert>

#include <unordered_map>

namespace res {

Resource::Ptr Mesh::from_yaml(const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto mesh = new Mesh(id, Mesh::tag(), name, File, path);

  mesh->populate(doc);

  return Resource::Ptr(mesh);
}

static const std::unordered_map<std::string, gx::Primitive> p_mesh_prims = {
  { "points", gx::Points },

  { "lines",     gx::Lines     },
  { "lineloop",  gx::LineLoop  },
  { "linestrip", gx::LineStrip },

  { "triangles",     gx::Triangles     },
  { "trianglefan",   gx::TriangleFan   },
  { "trianglestrip", gx::TriangleStrip },
};

void Mesh::populate(const yaml::Document& doc)
{
  auto vertex = doc("vertex");

  auto vertex_prop = [&](const char *name) {
    return vertex->get<yaml::Scalar>(name);
  };

  if(vertex_prop("normals")->b())  m_mesh.withNormals();
  if(vertex_prop("tangents")->b()) m_mesh.withTangents();

  if(auto colors = vertex_prop("colors")->ui())       m_mesh.withColors(colors);
  if(auto texcoords = vertex_prop("texcoords")->ui()) m_mesh.withTexCoords(texcoords);

  if(vertex_prop("bones")->b()) {
    m_mesh.withBoneWeights();
    m_mesh.withBoneIds();
  }

  auto it = p_mesh_prims.find(doc("primitive")->as<yaml::Scalar>()->str());
  assert(it != p_mesh_prims.end() && "Invalid primitive specified!");

  m_mesh.primitive(it->second);

}

}