#include <mesh/obj.h>

#include <util/str.h>

#include <cassert>
#include <cctype>
#include <cstdio>

#include <string>
#include <regex>

namespace mesh {

enum : char {
  Comment = '#',

  Vertex   = 'v',
  TexCoord = 't',
  Normal   = 'n',

  Face = 'f',
};

ObjLoader& ObjLoader::load(const char *data, size_t sz)
{
  util::splitlines(std::string(data, sz), [this](const std::string& line) {
    auto it = line.begin();
    while(it != line.end() && isspace(*it)) it++; // skip leading whitespace

    if(it == line.end() || *it == Comment) return; // Line is empty or a comment - skip it
   
    parseLine(it);
  });

  return *this;
}

const ObjMesh& ObjLoader::mesh() const
{
  return m_mesh;
}

void ObjLoader::parseLine(std::string::const_iterator it)
{
  char keyword = *it;
  it++;

  switch(keyword) {
  case Vertex:
    if(isspace(*it)) {
      m_mesh.m_v.push_back(loadVertex(it));
    } else {
      char ch = *it;
      it++;

      switch(ch) {
      case TexCoord: m_mesh.m_vt.push_back(loadTexCoord(it)); break;
      case Normal:   m_mesh.m_vn.push_back(loadNormal(it)); break;

      default: throw ParseError();
      }
    }
    break;

  case Face:
    m_mesh.m_tris.push_back(loadTriangle(it));
    break;

  default: throw ParseError();
  }
}

vec3 ObjLoader::loadVec3(std::string::const_iterator it)
{
  const char *str = &(*it);

  vec3 v;
  sscanf(str, "%f %f %f", &v.x, &v.y, &v.z);

  return v;
}

vec3 ObjLoader::loadVertex(std::string::const_iterator it)
{
  return loadVec3(it);
}

vec3 ObjLoader::loadTexCoord(std::string::const_iterator it)
{
  return loadVec3(it);
}

vec3 ObjLoader::loadNormal(std::string::const_iterator it)
{
  return loadVec3(it);
}

static const std::regex p_face(R"r((\d+)(?:\/(\d+)?\/(\d+))?)r", std::regex::optimize);

ObjMesh::Triangle ObjLoader::loadTriangle(std::string::const_iterator it)
{
  const char *str = &(*it);

  ObjMesh::Triangle triangle;
  auto num_verts = sscanf(str, "%u %u %u",
    &triangle[0].v,
    &triangle[1].v,
    &triangle[2].v);

  if(num_verts != 3) throw NonTriangularFaceError();

  // .obj indices are 1-based, but in C++ 0-based ones
  //   are preffered so convert them
  for(auto& v : triangle) {
    if(v.v  != ObjMesh::None) v.v--;
    if(v.vt != ObjMesh::None) v.vt--;
    if(v.vn != ObjMesh::None) v.vn--;
  }

  return triangle;
}

const std::vector<vec3>& ObjMesh::vertices() const
{
  return m_v;
}

const std::vector<ObjMesh::Triangle>& ObjMesh::faces() const
{
  return m_tris;
}

}