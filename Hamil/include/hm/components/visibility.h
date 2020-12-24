#pragma once

#include <hm/component.h>

#include <ek/visobject.h>

namespace hm {

struct Visibility : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */
  >;

  Visibility(u32 entity);

  static const Tag tag() { return "visibility"; }

  ek::VisibilityObject *visObject();
  const ek::VisibilityObject *visObject() const;

  ek::VisibilityObject vis;
};

}
