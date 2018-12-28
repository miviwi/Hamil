#pragma once

#include <hm/component.h>

#include <util/smallvector.h>
#include <mesh/mesh.h>

namespace hm {

struct Mesh : public Component {
  Mesh(u32 entity, const mesh::Mesh& mesh);

  static constexpr Tag tag() { return "Mesh"; }

  util::SmallVector<mesh::Mesh, 64> m;
};

}