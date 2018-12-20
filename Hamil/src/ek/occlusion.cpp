#include <ek/occlusion.h>

namespace ek {

OcclusionBuffer::OcclusionBuffer()
{
  m_fb = std::make_unique<float[]>(Size.area());

  m_obj_id  = std::make_unique<u16[]>(MaxTriangles);
  m_mesh_id = std::make_unique<u16[]>(MaxTriangles);
  m_bin     = std::make_unique<uint[]>(MaxTriangles);

  m_bin_sz = std::make_unique<u16[]>(NumBins);
}

OcclusionBuffer& OcclusionBuffer::binTriangles(const std::vector<VisibilityObject::Ptr>& objects)
{
  for(const auto& o : objects) {
    uint num_meshes = o->numMeshes();
    for(uint i = 0; i < num_meshes; i++) binTriangles(o->mesh(i));
  }

  return *this;
}

void OcclusionBuffer::binTriangles(const VisibilityMesh& mesh)
{
}

}