#pragma once

#include <res/res.h>
#include <res/resource.h>
#include <res/cache.h>

namespace res {

class HandleBase {
public:
  HandleBase(Resource::Id id);

protected:
  Resource::Ptr& lock();

  ResourceHandle m;
  Resource::Ptr m_locked;
};

// The resource is acquired through ResourceManager::handle(),
//   which means it must've been previously ResourceManager::load()'ed
//   or a NoSuchResourceError will be thrown
// Only to be used after res::init()
template <typename T>
class Handle : public HandleBase {
public:
  using HandleBase::HandleBase;

  // Returns 'nullptr' if the resource has expired
  T *operator->()
  {
    if(auto& r = lock()) return r->as<T>();

    return nullptr;
  }
};


}