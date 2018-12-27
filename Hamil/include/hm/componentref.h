#pragma once

#include <hm/hamil.h>

namespace hm {

// Wraps a 'Component *' to providde member access syntax sugar:
//   c->name().data() -> c().name().data()
template <typename T>
class ComponentRef {
public:
  using RefType = T;

  ComponentRef(T *component) :
    m(component)
  { }

  operator bool() const { return m; }

  T& get()
  {
    return *m;
  }

  const T& get() const
  {
    return *m;
  }

  T *ptr()
  {
    return m;
  }

  T& operator()()
  {
    return get();
  }

  const T& operator()() const
  {
    return get();
  }

private:
  T *m;
};

}

template <typename T>
using hmRef = hm::ComponentRef<T>;