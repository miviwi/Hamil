#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>

#include <util/hashindex.h>
#include <util/staticstring.h>

#include <array>
#include <vector>
#include <tuple>

namespace hm {

// Base class for Eugene.componentgen definitions.
//   - Derived classes (i.e. 'Components') must be declared
//     in the 'hm' namespace
//   - The derived class' (Component's) layout and size
//     determines what will be stored for each instance
//     in PrototypeChunks and be available for Systems
//   - Special static methods (some of which MUST be
//     overridden!) value's will be accessible from
//     ComponentMetaclasses and determine that for ex.
//     a Component is a 'Tag' i.e. dataless component
class Component {
public:
  using Tag = ComponentTag;

  // If a Component has no default constructor (one without parameters)
  //   this type must be redefined to a tuple of valid constructor args
  using ConstructorParamPack = std::tuple<>;

  Component();

  // - MUST be overridden by every Component (derived class)
  //   and be globally unique!
  static const Tag tag() { return "Component"; }

  // - Override only when necessary
  static constexpr u32 flags() { return 0; }

  // Only returns 'true' when it's Entity is != Invalid and alive()
  operator bool() const;

  void destroyed();

private:
  friend class IComponentStore;
};

}
