#pragma once

#include <hm/component.h>

#include <ek/visobject.h>

namespace hm {

struct Visibility : public Component {
  Visibility(u32 entity);

  static constexpr Tag tag() { return "visibility"; }

  ek::VisibilityObject *visObject();
  const ek::VisibilityObject *visObject() const;

  ek::VisibilityObject vis;
};

}