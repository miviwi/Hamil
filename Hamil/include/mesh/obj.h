#pragma once

#include <mesh/loader.h>

#include <math/geometry.h>

#include <string>
#include <array>
#include <vector>

namespace mesh {

class ObjMesh {
public:
  enum : uint {
    None = ~0u
  };

  struct Vertex {
    uint v  = None;
    uint vt = None;
    uint vn = None;
  };

  using Triangle = std::array<Vertex, 3>;

  const std::vector<vec3>& vertices() const;
  const std::vector<vec3>& normals() const;
  const std::vector<vec3>& texCoords() const;

  bool loaded() const;

  bool hasNormals() const;
  bool hasTexCoords() const;

  const std::vector<Triangle>& faces() const;

  AABB aabb() const;

private:
  friend class ObjLoader;

  AABB m_aabb = { vec3(INFINITY), vec3(-INFINITY) };

  std::vector<vec3> m_v;
  std::vector<vec3> m_vt;
  std::vector<vec3> m_vn;

  std::vector<Triangle> m_tris;
};

class ObjLoader : public MeshLoader {
public:
  ObjLoader& load(const void *data, size_t sz);

  // DO NOT chain this method through load() as it will
  //    cause a dangling refernce to be returned
  // The ObjLoader object must be kept around in order to 
  //   access the mesh(es)
  const ObjMesh& mesh() const;

protected:
  virtual MeshLoader& doLoad(const void *data, size_t sz);

  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt, gx::BufferHandle verts);
  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds);

  virtual MeshLoader& doStream(const gx::VertexFormat& fmt, gx::BufferHandle verts);
  virtual MeshLoader& doStreamIndexed(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds);

private:
  void parseLine(std::string::const_iterator it);

  vec3 loadVec3(std::string::const_iterator it);

  vec3 loadVertex(std::string::const_iterator it);
  vec3 loadTexCoord(std::string::const_iterator it);
  vec3 loadNormal(std::string::const_iterator it);

  ObjMesh::Triangle loadTriangle(std::string::const_iterator it);

  ObjMesh m_mesh;
};


}