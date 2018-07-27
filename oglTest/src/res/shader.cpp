#include <res/shader.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <util/format.h>

#include <unordered_map>
#include <utility>

namespace res {

class pShaderLibrary {
public:
  const char *get(const std::string& name) const
  {
    auto it = m_library.find(name);
    return it != m_library.end() ? it->second.data() : nullptr;
  }

  bool set(const std::string& name, std::string shader)
  {
    auto result = m_library.emplace(name, std::move(shader));

    return result.second;
  }

private:
  std::unordered_map<std::string, std::string> m_library;
};

static pShaderLibrary p_library;

Resource::Ptr Shader::from_yaml(const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto shader = new Shader(id, Shader::tag(), name, File, path);

  auto& vertex_sources   = shader->m_sources[Vertex];
  auto& geometry_sources = shader->m_sources[Geometry];
  auto& fragment_sources = shader->m_sources[Fragment];

  auto& inline_sources = shader->m_inline;

  auto get_ptr = [&](const yaml::Node::Ptr& src_node) -> const char *
  {
    auto src = src_node->as<yaml::Scalar>();

    const char *ptr = nullptr;
    if(src->tag() == "!inline") {
      inline_sources.emplace_back(src->str());

      ptr = inline_sources.back().data();
    } else if(src->tag() == "!lib") {
      ptr = p_library.get(src->str());

      if(!ptr) throw Error(util::fmt("shader '%s' doesn't exist!", src->str()));
    } else {
      throw Error(util::fmt("unknown shader source type '%s'", src->tag().value().data()));
    }

    return ptr;
  };

  auto do_stage = [&](const char *name, std::vector<const char *>& dst) -> void
  {
    if(auto stage = doc(name)) {
      for(const auto& src : *stage->as<yaml::Sequence>()) {
        auto ptr = get_ptr(src);
        dst.push_back(ptr);
      }
    }
  };

  do_stage("vertex", vertex_sources);
  do_stage("geometry", geometry_sources);
  do_stage("fragment", fragment_sources);

  shader->m_loaded  = true;

  return Resource::Ptr(shader);
}

bool Shader::hasStage(Stage stage) const
{
  return !m_sources[stage].empty();
}

const std::vector<const char *>& Shader::source(Stage stage) const
{
  return m_sources[stage];
}

}