#include <res/cache.h>

namespace res {

std::optional<ResourceCache::ResourcePtr> ResourceCache::probe(Resource::Id id)
{
  auto it = m.find(id);
  return it != m.end() ? it->second : {};
}

void ResourceCache::fill(const Resource::Ptr& r)
{
  m.insert({ r->id(), r });
}

}