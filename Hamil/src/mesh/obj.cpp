#include <mesh/obj.h>

#include <util/str.h>

#include <cassert>
#include <cctype>
#include <cstdio>

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

    if(*it == Comment) return; // Whole line is a comment - skip it
   
    parseLine(it);
  });

  return *this;
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
    m_mesh.m_tris.push_back(loadTraingle(it));
    break;

  default: throw ParseError();
  }
}

vec3 ObjLoader::loadVec3(std::string::const_iterator it)
{
  const char *str = &(*it);

  vec3 v;
  sscanf(str, "%f%f%f", &v.x, &v.y, &v.z);

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

ObjMesh::Triangle ObjLoader::loadTraingle(std::string::const_iterator it)
{
  return ObjMesh::Triangle();
}

}