#pragma once

#include <res/resource.h>

#include <util/staticstring.h>
#include <win32/file.h>
#include <mesh/mesh.h>
#include <mesh/loader.h>

#include <optional>

namespace yaml {
class Document;
}

namespace res {

class ResourceManager;

class Mesh : public Resource {
public:
  static constexpr Tag tag() { return "mesh"; }

  struct Error { };

  struct UnknownTypeError : public Error { };

  // 'doc' must be pre-validated!
  static Resource::Ptr from_yaml(const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");

  // Returns a MeshLoader appropriate for the Meshes file type
  //   that has had MeshLoader::loadParams() called on it
  //   if Resource::loaded() == true
  mesh::MeshLoader& loader();

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  // Returns the Meshes data location
  std::string populate(const yaml::Document& doc);

  mesh::Mesh m_mesh;
  mesh::MeshLoader *m_loader = nullptr;

  std::optional<win32::FileView> m_mesh_data = std::nullopt;
};

}
