#pragma once

#include <res/resource.h>

#include <util/staticstring.h>

#include <string>
#include <array>
#include <vector>

namespace yaml {
class Document;
}

namespace res {

class ResourceManager;

class Shader : public Resource {
public:
  static constexpr Tag tag() { return "shader"; }

  static constexpr util::StaticString InlineSource = "!inline";
  static constexpr util::StaticString ImportSource = "!import";
  static constexpr util::StaticString ExportSource = "!export";

  enum Stage : size_t {
    Global,
    Vertex, Geometry, Fragment,

    NumStages,
  };

  struct Error {
    const std::string what;
    Error(const std::string& what_) :
      what(what_)
    { }
  };

  // the const char *'s refer to an internal data structure which stores
  //   unique copies of shader sources (which are stiched together by gx::Shader)
  using Sources = std::array<std::vector<const char *>, NumStages>;

  bool hasStage(Stage stage) const;
  const std::vector<const char *>& source(Stage stage) const;

  // 'doc' must be pre-validated!
  static Resource::Ptr from_yaml(const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");

protected:
  using Resource::Resource;

private:
  friend ResourceManager;
  
  Sources m_sources;
  std::vector<std::string> m_inline;
};

}