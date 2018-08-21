#pragma once

#include <res/res.h>
#include <res/resource.h>
#include <res/manager.h>
#include <res/cache.h>

namespace res {

// Only to be used after res::init()
template <typename T>
class Handle {
public:
  Handle(Resource::Id id) :
    m(resource().handle(id))
  { }

  T *operator->() const
  {
    if(auto r = m.lock()) return r->as<T>();

    return nullptr;
  }

private:
  ResourceHandle m;
};


}