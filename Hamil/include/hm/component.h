#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace hm {

// Base class for Eugene/ComponentGen definitions.
//   - Derived classes (i.e. 'Components') must be declared
//     in the 'hm' namespace
class Component {
public:
  Component(Entity e);
  
  Entity entity() const;

  // Only returns 'true' when it's Entity is != Invalid and alive()
  operator bool() const;

  void destroyed();

private:
  friend class IComponentStore;

  Entity m_entity;
};

}