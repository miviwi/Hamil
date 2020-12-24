#pragma once

#include <hm/component.h>

#include <util/smallvector.h>
#include <mesh/mesh.h>

namespace hm {

struct Mesh : public Component {
  using ConstructorParamPack = std::tuple<
    u32 /* entity */, mesh::Mesh
  >;

  Mesh(u32 entity, const mesh::Mesh& mesh);

  static const Tag tag() { return "Mesh"; }

  util::SmallVector<mesh::Mesh, 64> m;
};

}
