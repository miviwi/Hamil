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
  using ImportSet = std::unordered_set<std::string>;

  const char *get(const std::string& name) const
  {
    auto it = m_library.find(name);
    return it != m_library.end() ? it->second.data() : nullptr;
  }

  // Returns 'true' when 'name' was never exported before
  bool exportSource(const std::string& name, std::string shader)
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

  const ImportSet& getImports(const std::string& shader)
  {
    return m_imports[shader];
  }

private:
  // Stores concatenated inline (non-imported) sources
  //   whose parent stage is marked as !export
  std::unordered_map<std::string, std::string> m_library;

  // std::unordered_set is used here because addImports()
  //   calls itself recursively to add all dependencies
  //   of a given Shader which could result in duplicates.
  //   Sets store only unique keys which prevents this from
  //   happening
  std::unordered_map<std::string, ImportSet> m_imports;
};

static pShaderLibrary p_library;

Resource::Ptr Shader::from_yaml(const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto shader = new Shader(id, Shader::tag(), name, File, path);

  // Process all possible shader stages and the 'Global'
  //   pseudo-stage used for imports/exports
  shader->doStage(doc, Global, "glsl")
    .doStage(doc, Vertex, "vertex")
    .doStage(doc, Geometry, "geometry")
    .doStage(doc, Fragment, "fragment");

  shader->m_loaded = true;

  return Resource::Ptr(shader);
}

Shader& Shader::doStage(const yaml::Document& doc, Stage stage_id, const char *stage_name)
{
  auto stage_node = doc(stage_name);
  if(!stage_node) return *this;

  const yaml::Sequence *stage = stage_node->as<yaml::Sequence>();
  auto& dst = m_sources[stage_id];
  auto& inline_src = m_inline[stage_id];

  // format the name as
  //      <program>.(glsl|vert|geom|frag)
  // ex.
  //   wireframe.geom, blinnphong.frag, util.glsl, ...
  std::string full_name = util::fmt("%s.%.4s", name(), stage_name);

  // Conservatively allocate buckets
  pShaderLibrary::ImportSet imports(stage->size());

  // Resolve imports
  for(const auto& src : *stage) {
    if(!(src->tag() == ImportSource)) continue;

    auto import_name = src->as<yaml::Scalar>()->str();
    const auto& imports_imports = p_library.getImports(import_name);

    // Import the requested source
    imports.emplace(import_name);
    // ...and it's dependencies - the container (std::unordered_set)
    //    ensures that a given source gets included only once
    imports.insert(imports_imports.cbegin(), imports_imports.cend());

    // pShaderLibrary::addImports() adds 'import_name' and
    //   all of it's dependencies recursively into our
    //   std::unordered_set of imports
    p_library.addImports(full_name, import_name);
  }

  for(const auto& import : imports) {   // Append all import sources first...
    auto ptr = p_library.get(import);

    if(!ptr) throw Error(util::fmt("shader '%s' doesn't exist!", import));
    dst.push_back(ptr);
  }

  for(const auto& src : *stage) {       // ...then stitch together !inline nodes
    bool is_inline = (src->tag() == InlineSource);
    if(!is_inline) continue;

    auto str = src->as<yaml::Scalar>()->str();
    inline_src += str;
  }

  if(stage->tag() == ExportSource) {
    // Only our inline sources (!inline nodes) are
    //   exported to the library, so that importing
    //   Shaders can avoid double-includes when
    //   they share dependencies with the importee
    p_library.exportSource(full_name, inline_src);
  }

  dst.push_back(inline_src.data());     // ...and finally append them as well

  return *this;
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