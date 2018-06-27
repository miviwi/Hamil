#include <res/cache.h>

namespace res {

std::optional<ResourceCache::ResourcePtr> ResourceCache::probe(Resource::Id id)
{
#if 0
  auto it = m.find(id);
  return it != m.end() ? it->second : {};
#endif
  return {};
}

void ResourceCache::fill(const Resource::Ptr& r)
{
  m.insert({ r->id(), r });
}

}