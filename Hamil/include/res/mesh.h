#pragma once

#include <res/resource.h>

#include <util/staticstring.h>
#include <mesh/mesh.h>

namespace yaml {
class Document;
}

namespace res {

class ResourceManager;

class Mesh : public Resource {
public:
  static constexpr Tag tag() { return "mesh"; }

  struct Error { };

  // 'doc' must be pre-validated!
  static Resource::Ptr from_yaml(const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  void populate(const yaml::Document& doc);

  mesh::Mesh m_mesh;
};

}
