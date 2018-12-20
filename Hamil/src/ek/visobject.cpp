#include <ek/visobject.h>

namespace ek {

const AABB& VisibilityObject::aabb() const
{
  return m_aabb;
}

VisibilityMesh& VisibilityObject::mesh(uint idx)
{
  return m_meshes.at(idx);
}

const VisibilityMesh& VisibilityObject::mesh(uint idx) const
{
  return m_meshes.at(idx);
}

uint VisibilityObject::numMeshes() const
{
  return m_meshes.size();
}

VisibilityObject& VisibilityObject::transformMeshes(const mat4& viewprojectionviewport)
{
  // Transform all the meshes into screen space
  for(auto& mesh : m_meshes) {
    auto mvp = viewprojectionviewport * mesh.model;
    transformOne(mesh, mvp);
  }

  return *this;
}

void VisibilityObject::transformOne(VisibilityMesh& mesh, const mat4& mvp)
{
  auto in  = mesh.verts;
  auto out = mesh.xformed.get();
  for(size_t i = 0; i < mesh.num_verts; i++) {
    auto v = vec4(*in++, 1.0f);
    v = mvp * v;

    // Check if the vertex isn't clipped by the near plane
    if(v.z <= v.w) {
      *out++ = v.perspectiveDivide();
    } else {
      *out++ = vec4::zero();  // If it is set all of it's coords to 0
    }
  }
}

VisibilityMesh::Triangle VisibilityMesh::gatherTri(uint idx)
{
  auto off = idx*3;
  u16 inds[3] = { inds[off + 0], inds[off + 1], inds[off + 2] };

  return { xformed[inds[0]], xformed[inds[1]], xformed[inds[2]] };
}

}