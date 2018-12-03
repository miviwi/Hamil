#include <mesh/mesh.h>

namespace mesh {

Mesh& Mesh::withNormals()
{
  vertex_components |= Normal;

  return *this;
}

Mesh& Mesh::withTangents()
{
  vertex_components |= Tangent;

  return *this;
}

Mesh& Mesh::withColors(u8 num)
{
  vertex_components |= Color;
  num_vertex_colors = num;

  return *this;
}

Mesh& Mesh::withTexCoords(u8 num)
{
  vertex_components |= TexCoord;
  num_vertex_tex_coords = num;

  return *this;
}

Mesh& Mesh::withBoneWeights()
{
  vertex_components |= BoneWeights;

  return *this;
}

Mesh& Mesh::withBoneIds()
{
  vertex_components |= BoneIds;

  return *this;
}

Mesh& Mesh::withArray(gx::ResourcePool::Id id)
{
  vertex_array_flags &= ~Indexed;
  vertex_array_id = id;

  return *this;
}

Mesh& Mesh::withIndexedArray(gx::ResourcePool::Id id)
{
  vertex_array_flags |= Indexed;
  vertex_array_id = id;

  return *this;
}

Mesh& Mesh::primitive(gx::Primitive p)
{
  array_primitive = (u16)p;

  return *this;
}

Mesh& Mesh::withBase(u32 b)
{
  base = b;

  return *this;
}

Mesh& Mesh::withOffset(u32 off)
{
  offset = off;

  return *this;
}

Mesh& Mesh::withNum(u32 n)
{
  num = n;

  return *this;
}

u64 Mesh::formatId() const
{
  u32 components = (u32)num_vertex_colors | ((u32)num_vertex_tex_coords<<8);
  u32 vertex_props = vertex_components | (components<<16);
  u32 array_props  =  vertex_array_flags;
  
  return (u64)vertex_props | ((u64)array_props<<32ull);
}

gx::Primitive Mesh::getPrimitive() const
{
  return (gx::Primitive)array_primitive;
}

bool Mesh::isIndexed() const
{
  return vertex_array_flags & Indexed;
}

}