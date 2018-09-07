#pragma once

#include <hm/hamil.h>

namespace hm {

// Wraps a 'Component *' to providde member access syntax sugar:
//   c->name().data() -> c().name().data()
template <typename T>
class ComponentRef {
public:
  ComponentRef(T *component) :
    m(component)
  { }

  operator bool() { return m; }

  T& get()
  {
    return *m;
  }

  T& operator()()
  {
    return get();
  }

private:
  T *m;
};

}