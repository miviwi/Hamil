#pragma once

#include <hm/component.h>

#include <gx/resourcepool.h>
#include <gx/program.h>

namespace hm {

struct Model : public Component {
  Model(u32 entity);

  static constexpr Tag tag() { return "Model"; }

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

  Model& withNormals();
  Model& withTangents();
  Model& withColors(u8 num);
  Model& withTexCoords(u8 num);

  Model& withBoneWeights();
  Model& withBoneIds();

  Model& withArray(gx::ResourcePool::Id id);
  Model& withIndexedArray(gx::ResourcePool::Id id);

  Model& primitive(gx::Primitive p);

  Model& withBase(u32 b);
  Model& withOffset(u32 off);
  Model& withNum(u32 n);

  u16 vertex_components = 0;
  u8 num_vertex_colors = 0;
  u8 num_vertex_tex_coords = 0;

  gx::ResourcePool::Id vertex_array_id = gx::ResourcePool::Invalid;

  u16 vertex_array_flags = 0;
  u16 array_primitive = (u16)gx::Triangles;
  u32 base = None, offset = None, num = 0;
};

}