#include <hm/components/mesh.h>

namespace hm {

Mesh::Mesh(u32 entity, const mesh::Mesh& mesh) :
  Component(entity),
  m(mesh)
{
}

}