#pragma once

#include <common.h>

#include <math/geometry.h>

#include <string>
#include <array>
#include <vector>

namespace mesh {

class ObjMesh {
public:
  struct Vertex {
    uint v, vt, vn;
  };

  using Triangle = std::array<Vertex, 3>;

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

  ObjLoader& load(const char *data, size_t sz);

private:
  void parseLine(std::string::const_iterator it);

  vec3 loadVec3(std::string::const_iterator it);

  vec3 loadVertex(std::string::const_iterator it);
  vec3 loadTexCoord(std::string::const_iterator it);
  vec3 loadNormal(std::string::const_iterator it);

  ObjMesh::Triangle loadTraingle(std::string::const_iterator it);

  ObjMesh m_mesh;
};


}