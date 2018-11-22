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

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  mesh::Mesh m_mesh;
};

}
