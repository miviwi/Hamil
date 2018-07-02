#include <res/cache.h>

namespace res {

ResourceCache::ResourceCache(Type type) :
  m_type(type)
{
}

std::optional<ResourceCache::ResourcePtr> ResourceCache::probe(Resource::Id id)
{
  auto it = m.find(id);
  return it != m.end() ? std::optional<ResourcePtr>(it->second) : std::nullopt;
}

ResourceCache::ResourcePtr ResourceCache::fill(const Resource::Ptr& r)
{
  auto result = m.emplace(r->id(), r);
  if(result.second) return ResourcePtr(result.first->second);

  return ResourcePtr();
}

}