#include <res/manager.h>
#include <util/hash.h>

#include <utility>

namespace res {

ResourceManager::ResourceManager(std::initializer_list<Resource::Tag> tags,
  std::initializer_list<ResourceLoader *> loader_chain) :
  m_loader_chain(loader_chain.size())
{
  for(const auto& tag : tags) m_caches.insert({ tag, ResourceCache() });

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

ResourceCache::ResourcePtr ResourceManager::load(Resource::Tag tag, Resource::Id id, LoadFlags flags)
{
  auto& cache = m_caches.find(tag)->second;
  if(auto r = cache.probe(id)) return *r;

  for(auto& loader : m_loader_chain) {
    if(auto r = loader->load(tag, id, flags)) return r;
  }
  throw Error(); // if no loader manages to locate the resource throw :(

  return ResourceCache::ResourcePtr(); // unreachable
}

}