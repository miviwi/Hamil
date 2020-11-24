#pragma once

#include <common.h>

#include <gx/gx.h>
#include <gx/resourcepool.h>

namespace mesh {

struct Mesh {
  enum VertexComponent : u16 {
    Normal   = (1<<0),
    Tangent  = (1<<1),
    Color    = (1<<2),
    TexCoord = (1<<3),

    BoneWeights = (1<<4),
    BoneIds     = (1<<5),
  };

  enum VertexArrayFlags : u16 {
    Indexed = (1<<0),
  };

  enum : u32 {
    None = ~0u,
  };

  Mesh& withNormals();
  Mesh& withTangents();
  Mesh& withColors(u8 num);
  Mesh& withTexCoords(u8 num);

  Mesh& withBoneWeights();
  Mesh& withBoneIds();

  Mesh& withArray(gx::ResourcePool::Id id);
  Mesh& withIndexedArray(gx::ResourcePool::Id id);

  Mesh& primitive(gx::Primitive p);

  Mesh& withBase(u32 b);
  Mesh& withOffset(u32 off);
  Mesh& withNum(u32 n);

  // Integer which can be used as an id
  //   for a vertex format which fits this mesh,
  //   and will be the same for compatible meshes
  u64 formatId() const;

  gx::Primitive getPrimitive() const;

  bool isIndexed() const;

  u16 vertex_components = 0;
  u8 num_vertex_colors = 0;
  u8 num_vertex_tex_coords = 0;

  gx::ResourcePool::Id vertex_array_id = gx::ResourcePool::Invalid;

  u16 vertex_array_flags = 0;
  u16 array_primitive = (u16)gx::Primitive::Triangles;
  u32 base = None, offset = None, num = 0;
};

}
