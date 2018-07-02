#pragma once

#include <common.h>
#include <res/resource.h>

#include <unordered_map>
#include <memory>
#include <optional>

namespace res {

class ResourceCache {
public:
  using Map = std::unordered_map<Resource::Id, Resource::Ptr>;
  using ResourcePtr = std::weak_ptr<Resource>;

  enum Type {
    Generic, Static,

    NumTypes,
  };

  ResourceCache(Type type);

  std::optional<ResourcePtr> probe(Resource::Id id);
  ResourcePtr fill(const Resource::Ptr& r);

private:
  Type m_type;
  Map m;
};

using ResourceHandle = ResourceCache::ResourcePtr;

}