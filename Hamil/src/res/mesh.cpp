#include <res/mesh.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <gx/gx.h>
#include <mesh/obj.h>

#include <cassert>

#include <unordered_map>
#include <functional>

namespace res {

Resource::Ptr Mesh::from_yaml(const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto mesh = new Mesh(id, Mesh::tag(), name, File, path);

  auto location = mesh->populate(doc);

  // TODO:
  //   1. Read the mesh data from the disk
  //   2. Parse it
  //   3. Stream it into a <Indexed>VertexArray
  win32::File mesh_data(location.data(), win32::File::Read, win32::File::OpenExisting);
  mesh->m_mesh_data.emplace(mesh_data.map(win32::File::ProtectRead));

  mesh->m_loader->loadParams(mesh->m_mesh_data->get(), mesh_data.size());

  mesh->m_loaded = true;

  return Resource::Ptr(mesh);
}

mesh::MeshLoader& Mesh::loader()
{
  return *m_loader;
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

using MeshLoaderFactoryFn = std::function<mesh::MeshLoader *()>;
static const std::unordered_map<std::string, MeshLoaderFactoryFn> p_loader_factories = {
  { "obj", []() -> mesh::MeshLoader * { return new mesh::ObjLoader(); } },
};

std::string Mesh::populate(const yaml::Document& doc)
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

  return location_str;
}

}