#include <res/shader.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <util/format.h>

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <utility>
#include <sstream>

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

  void addImports(const std::string& name, const std::string& shader)
  {
    auto& imports = m_imports[name];
    auto import_result = imports.emplace(shader);
    if(!import_result.second) return;

    auto shader_imports_it = m_imports.find(shader);

    // Check if we need to recurse...
    if(shader_imports_it == m_imports.end()) return;

    for(auto& shader_import : shader_imports_it->second) {
      addImports(name, shader_import);  // Recursively import
    }
  }

  const std::unordered_set<std::string>& getImports(const std::string& shader)
  {
    return m_imports[shader];
  }

private:
  std::unordered_map<std::string, std::string> m_library;
  std::unordered_map<std::string,
    std::unordered_set<std::string>> m_imports;
};

static pShaderLibrary p_library;

Resource::Ptr Shader::from_yaml(const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto shader = new Shader(id, Shader::tag(), name, File, path);

  auto& global_sources   = shader->m_sources[Global];
  auto& vertex_sources   = shader->m_sources[Vertex];
  auto& geometry_sources = shader->m_sources[Geometry];
  auto& fragment_sources = shader->m_sources[Fragment];

  auto& inline_sources = shader->m_inline;

  auto get_ptr = [&](const yaml::Node::Ptr& src_node) -> const char *
  {
    auto src = src_node->as<yaml::Scalar>();

    const char *ptr = nullptr;
    if(src->tag() == InlineSource) {
      inline_sources.emplace_back(src->str());

      ptr = inline_sources.back().data();
    } else {
      throw Error(util::fmt("unknown shader source type '%s'", src->tag().value()));
    }

    return ptr;
  };

  auto export_source = [&](const std::string& lib_name, std::vector<std::string>& src) -> void
  {
    std::ostringstream ss;
    for(const auto& s : src) ss << s;

    p_library.set(lib_name, ss.str());
  };

  auto do_stage = [&](const char *stage_name, std::vector<const char *>& dst)
  {
    auto stage_node = doc(stage_name);
    if(!stage_node) return;

    const yaml::Sequence *stage = stage_node->as<yaml::Sequence>();

    // format the name as <program>.(glsl|vert|geom|frag)
    // ex.
    //     - wireframe.geom
    //     - blinnphong.frag
    std::string full_name = util::fmt("%s.%.4s", name, stage_name);

    // Conservatively allocate buckets
    std::unordered_set<std::string> imports(stage->size());

    // Resolve imports
    for(const auto& src : *stage) {
      if(!(src->tag() == ImportSource)) continue;

      auto import_name = src->as<yaml::Scalar>()->str();
      const auto& imports_imports = p_library.getImports(import_name);

      imports.emplace(import_name);
      imports.insert(imports_imports.cbegin(), imports_imports.cend());

      p_library.addImports(full_name, import_name);
    }

    // Append all import sources first...
    for(const auto& import : imports) {
      auto ptr = p_library.get(import);

      if(!ptr) throw Error(util::fmt("shader '%s' doesn't exist!", import));
      dst.push_back(ptr);
    }

    // ...and then the rest
    for(const auto& src : *stage) {
      // Imports have already been resolved
      if(src->tag() == ImportSource) continue;

      dst.push_back(get_ptr(src));
    }

    if(stage->tag() == ExportSource) {
      export_source(full_name, inline_sources);
    }
  };

  do_stage("glsl", global_sources);
  do_stage("vertex", vertex_sources);
  do_stage("geometry", geometry_sources);
  do_stage("fragment", fragment_sources);

  shader->m_loaded = true;

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