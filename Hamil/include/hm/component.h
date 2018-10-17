#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>

#include <util/hashindex.h>
#include <util/staticstring.h>

#include <array>
#include <vector>

namespace hm {

struct GameObject;

// Base class for Eugene/ComponentGen definitions.
//   - Derived classes (i.e. 'Components') must be declared
//     in the 'hm' namespace
class Component {
public:
  using Tag = util::StaticString;

  Component(Entity e);

  static constexpr Tag tag() { return "Component"; }
  
  Entity entity() const;
  GameObject& gameObject() const;

  // Only returns 'true' when it's Entity is != Invalid and alive()
  operator bool() const;

  void destroyed();

private:
  friend class IComponentStore;

  Entity m_entity;
};

}