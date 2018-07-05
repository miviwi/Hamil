#pragma once

#include <res/resource.h>

#include <string>
#include <array>

namespace res {

class ShaderResource : public Resource {
public:
  static constexpr Tag tag() { return "shader"; }

  enum Stage : size_t {
    Vertex, Geometry, Fragment,

    NumStages,
  };

  using Sources = std::array<std::string, NumStages>;

  bool hasStage(Stage stage) const;
  const std::string& source(Stage stage) const;

  static Resource::Ptr from_file(Sources&& sources, Id id,
    const std::string& name = "", const std::string& path = "");

protected:
  using Resource::Resource;

private:
  friend class ResourceManager;
  
  Sources m_sources;
};

}