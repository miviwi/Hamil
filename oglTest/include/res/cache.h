#pragma once

#include <common.h>
#include <res/resource.h>

#include <unordered_map>
#include <memory>
#include <optional>

namespace res {

class ResourceCache {
public:
  using Map = std::unordered_map<Resource::Id, Resource::Ptr, Resource::Hash, Resource::Compare>;
  using ResourcePtr = std::weak_ptr<Resource>;

  std::optional<ResourcePtr> probe(Resource::Id id);
  void fill(const Resource::Ptr& r);

private:
  Map m;
};

}