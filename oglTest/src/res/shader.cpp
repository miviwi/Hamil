#include <res/shader.h>

#include <utility>

namespace res {
Resource::Ptr ShaderResource::from_file(Sources&& sources, Id id, const std::string& name, const std::string& path)
{
  auto shader = new ShaderResource(id, ShaderResource::tag(), name, File, path);

  shader->m_sources = std::move(sources);
  shader->m_loaded  = true;

  return Resource::Ptr(shader);
}

bool ShaderResource::hasStage(Stage stage) const
{
  return !m_sources[stage].empty();
}

const std::string& ShaderResource::source(Stage stage) const
{
  return m_sources[stage];
}

}