#include <res/mesh.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <gx/gx.h>
#include <mesh/obj.h>

#include <cassert>

#include <unordered_map>
#include <functional>

namespace res {

Resource::Ptr Mesh::from_yaml(IOBuffer mesh_data,
  const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto mesh = new Mesh(id, Mesh::tag(), name, File, path);

  mesh->m_mesh_data = std::move(mesh_data);
  mesh->populate(doc);

  // TODO:
  //   1. Read the mesh data from the disk
  //   2. Parse it
  //   3. Stream it into a <Indexed>VertexArray
  mesh->m_loader->loadParams(mesh->m_mesh_data.get(), mesh->m_mesh_data.size());

  return Resource::Ptr(mesh);
}

mesh::MeshLoader& Mesh::loader()
{
  return *m_loader;
}

const mesh::Mesh& Mesh::mesh() const
{
  return m_mesh;
}

static const std::unordered_map<std::string, gx::Primitive> p_mesh_prims = {
  { "points", gx::Primitive::Points },

  { "lines",     gx::Primitive::Lines     },
  { "lineloop",  gx::Primitive::LineLoop  },
  { "linestrip", gx::Primitive::LineStrip },

  { "triangles",     gx::Primitive::Triangles     },
  { "trianglefan",   gx::Primitive::TriangleFan   },
  { "trianglestrip", gx::Primitive::TriangleStrip },
};

using MeshLoaderFactoryFn = std::function<mesh::MeshLoader *()>;
static const std::unordered_map<std::string, MeshLoaderFactoryFn> p_loader_factories = {
  { "obj", []() -> mesh::MeshLoader * { return new mesh::ObjLoader(); } },
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

  auto location = doc("location");
  std::string location_str = location->as<yaml::Scalar>()->str();

  auto mesh_type_off = location_str.rfind('.');
  assert(mesh_type_off != std::string::npos && "Invalid Mesh file type!");

  auto loader_factory = p_loader_factories.find(location_str.substr(mesh_type_off+1));
  if(loader_factory == p_loader_factories.end()) throw UnknownTypeError();
  
  m_loader = loader_factory->second();

  m_loader->onLoaded([this](mesh::MeshLoader& loader) {
    m_mesh_data.release();   // Dispose of the mesh data when it's no longer needed

    m_loaded = true;
  });
}

}
