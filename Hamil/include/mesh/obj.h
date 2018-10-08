#pragma once

#include <common.h>

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

  const std::vector<Triangle>& faces() const;

private:
  friend class ObjLoader;

  std::vector<vec3> m_v;
  std::vector<vec3> m_vt;
  std::vector<vec3> m_vn;

  std::vector<Triangle> m_tris;
};

class ObjLoader {
public:
  struct Error {
  };

  struct ParseError : public Error {
  };

  struct NonTriangularFaceError : Error {
  };

  ObjLoader& load(const char *data, size_t sz);

  // DO NOT chain this method through load() as it will
  //    cause a dangling refernce to be returned
  // The ObjLoader object must be kept around in order to 
  //   access the mesh(es)
  const ObjMesh& mesh() const;

private:
  void parseLine(std::string::const_iterator it);

  vec3 loadVec3(std::string::const_iterator it);

  vec3 loadVertex(std::string::const_iterator it);
  vec3 loadTexCoord(std::string::const_iterator it);
  vec3 loadNormal(std::string::const_iterator it);

  // TODO!
  ObjMesh::Triangle loadTriangle(std::string::const_iterator it);

  ObjMesh m_mesh;
};


}