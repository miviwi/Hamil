#include <mesh/obj.h>

#include <util/str.h>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <string>

namespace mesh {

enum : char {
  Comment = '#',

  Object   = 'o',
  Group    = 'g',
  Shading  = 's',
  Material = 'm',
  UseMtl   = 'u',

  Vertex   = 'v',
  TexCoord = 't',
  Normal   = 'n',

  Face = 'f',
};

enum : size_t {
  InitialVertices  = 4096,
  InitialTexCoords = 4096,
  InitialNormals   = 4096,

  InitialFaces = 4096,
};

ObjLoader& ObjLoader::load(const char *data, size_t sz)
{
  m_mesh.m_v.reserve(InitialVertices);
  m_mesh.m_vt.reserve(InitialTexCoords);
  m_mesh.m_vn.reserve(InitialNormals);

  m_mesh.m_tris.reserve(InitialVertices);

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

  case Object: break; // TODO
  case Group: break;

  case Shading: break;   //  Ignore
  case Material: break;  // -- || --
  case UseMtl: break;    // -- || -- 

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

ObjMesh::Triangle ObjLoader::loadTriangle(std::string::const_iterator str_it)
{
  const char *str = &(*str_it);

  ObjMesh::Triangle triangle;

  const char *pos = str;
  auto it = triangle.begin();
  while(*pos) {
    char *end = nullptr;
    auto read_index = [&]() -> uint {
      return std::strtoul(pos, &end, 0);
    };

    while(isspace(*pos)) pos++;
    if(*pos == '\0') break;

    // Too many vertices
    if(it == triangle.end()) throw NonTriangularFaceError();

    it->v = read_index();

    if(*end == '/') {
      pos = end+1;
      if(*pos != '/') {
        pos = end;
        it->vt = read_index();
      } else {
        end = (char *)pos;
      }
    }

    if(*end == '/') {
      pos = end+1;
      it->vn = read_index();
    }

    pos = end+1;
    it++;
  }

  // Not enough vertices
  if(it != triangle.end()) throw NonTriangularFaceError();

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

const std::vector<vec3>& ObjMesh::normals() const
{
  return m_vn;
}

const std::vector<vec3>& ObjMesh::texCoords() const
{
  return m_vt;
}

const std::vector<ObjMesh::Triangle>& ObjMesh::faces() const
{
  return m_tris;
}

}