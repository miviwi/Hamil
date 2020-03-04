#pragma once

#include <res/resource.h>
#include <res/io.h>

#include <util/staticstring.h>
#include <win32/file.h>
#include <mesh/mesh.h>
#include <mesh/loader.h>

#include <optional>
#include <utility>

namespace yaml {
class Document;
}

namespace res {

class ResourceManager;

class Mesh : public Resource {
public:
  static const Tag tag() { return "mesh"; }

  struct Error { };

  struct UnknownTypeError : public Error { };

  // 'doc' must be pre-validated!
  static Resource::Ptr from_yaml(IOBuffer mesh_data,
    const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");

  // Returns a MeshLoader appropriate for the Meshes file type
  //   that has had MeshLoader::loadParams() called on it
  //   if Resource::loaded() == true
  mesh::MeshLoader& loader();

  const mesh::Mesh& mesh() const;

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  void populate(const yaml::Document& doc);

  mesh::Mesh m_mesh;
  mesh::MeshLoader *m_loader = nullptr;

  IOBuffer m_mesh_data;
};

}
