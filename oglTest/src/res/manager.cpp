#include <res/manager.h>
#include <res/text.h>
#include <res/shader.h>

#include <util/hash.h>

#include <map>
#include <utility>

namespace res {

ResourceManager::ResourceManager(std::initializer_list<ResourceLoader *> loader_chain) :
  m_caches({ { ResourceCache::Generic }, { ResourceCache::Static } }),
  m_loader_chain(loader_chain.size())
{
  // populate the loader chain in reverse to allow
  // listing the loaders in a natural order (the last loader
  // in the std::initializer_list has the highest priority)
  size_t i = loader_chain.size()-1;
  for(const auto& loader : loader_chain) {
    m_loader_chain[i] = ResourceLoader::Ptr(loader);
    i--;
  }
}

Resource::Id ResourceManager::guid(Resource::Tag tag, const std::string& name, const std::string& path) const
{
  size_t hash = 0;
  util::hash_combine<Resource::Tag::Hash>(hash, tag);
  util::hash_combine<util::XXHash<std::string>>(hash, name);
  util::hash_combine<util::XXHash<std::string>>(hash, path);

  return hash;
}

ResourceHandle ResourceManager::load(Resource::Id id, LoadFlags flags)
{
  auto& cache = getCache(flags);
  if(auto r = cache.probe(id)) return *r;

  for(auto& loader : m_loader_chain) {
    if(auto r = loader->load(id, flags)) return cache.fill(r);
  }
  throw Error(); // if no loader manages to locate the resource - throw :(

  return ResourceHandle(); // unreachable
}

ResourceHandle ResourceManager::handle(Resource::Id id)
{
  for(auto& cache : m_caches) {
    if(auto r = cache.probe(id)) return r.value();
  }
  throw NoSuchResourceError(id);

  return ResourceHandle(); // unreachable
}

static const std::map<std::string, res::Resource::Tag> p_tags = {
  { TextResource::tag().get(),   TextResource::tag() },
  { ShaderResource::tag().get(), ShaderResource::tag() },
};

std::optional<Resource::Tag> ResourceManager::make_tag(const char *tag)
{
  auto it = p_tags.find(tag);
  return it != p_tags.end() ? std::optional<Resource::Tag>(it->second) : std::nullopt;
}

ResourceCache& ResourceManager::getCache(LoadFlags flags)
{
  size_t idx = flags & LoadStatic ? ResourceCache::Static : ResourceCache::Generic;

  return m_caches[idx];
}

}