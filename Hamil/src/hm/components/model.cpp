#include <hm/components/model.h>

namespace hm {

Model::Model(u32 entity) :
  Component(entity)
{
}

Model& Model::withNormals()
{
  vertex_components |= Normal;

  return *this;
}

Model& Model::withTangents()
{
  vertex_components |= Tangent;

  return *this;
}

Model& Model::withColors(u8 num)
{
  vertex_components |= Color;
  num_vertex_colors = num;

  return *this;
}

Model& Model::withTexCoords(u8 num)
{
  vertex_components |= TexCoord;
  num_vertex_tex_coords = num;

  return *this;
}

Model& Model::withBoneWeights()
{
  vertex_components |= BoneWeights;

  return *this;
}

Model& Model::withBoneIds()
{
  vertex_components |= BoneIds;

  return *this;
}

Model& Model::withArray(gx::ResourcePool::Id id)
{
  vertex_array_flags &= ~Indexed;
  vertex_array_id = id;

  return *this;
}

Model& Model::withIndexedArray(gx::ResourcePool::Id id)
{
  vertex_array_flags |= Indexed;
  vertex_array_id = id;

  return *this;
}

Model& Model::primitive(gx::Primitive p)
{
  array_primitive = (u16)p;

  return *this;
}

Model& Model::withBase(u32 b)
{
  base = b;

  return *this;
}

Model& Model::withOffset(u32 off)
{
  offset = off;

  return *this;
}

Model& Model::withNum(u32 n)
{
  num = n;

  return *this;
}

}