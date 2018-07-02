#pragma once

#include <common.h>
#include <res/resource.h>
#include <res/loader.h>
#include <res/cache.h>

#include <string>
#include <vector>
#include <initializer_list>
#include <memory>
#include <type_traits>

namespace res {

class ResourceManager {
public:
  using Ptr = std::unique_ptr<ResourceManager>;

  struct Error { };

  ResourceManager(std::initializer_list<ResourceLoader *> loader_chain);

  template <typename T>
  Resource::Id guid(const std::string& name, const std::string& path)
  {
    static_assert(Resource::is_resource<T>(), "T must be derived from Resource!");
    return guid(T::tag(), name, path);
  }
  Resource::Id guid(Resource::Tag tag, const std::string& name, const std::string& path) const;

  ResourceCache::ResourcePtr load(Resource::Id id, LoadFlags flags = LoadDefault);

private:
  ResourceCache& getCache(LoadFlags flags);

  std::vector<ResourceCache> m_caches;
  std::vector<ResourceLoader::Ptr> m_loader_chain;
};

}