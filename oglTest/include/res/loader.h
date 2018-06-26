#pragma once

#include <common.h>
#include <res/resource.h>

#include <memory>

namespace res {

enum LoadFlags {
  LoadDefault, LoadStatic, Precache
};

class ResourceLoader {
public:
  using Ptr = std::unique_ptr<ResourceLoader>;

  virtual Resource::Ptr load(Resource::Tag tag, Resource::Id id, LoadFlags flags) = 0;
};

}