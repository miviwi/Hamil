#include <mesh/obj.h>

#include <util/str.h>
#include <gx/buffer.h>

#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <string>
#include <tuple>

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

ObjLoader& ObjLoader::load(const void *data, size_t sz)
{
  doLoad(data, sz);

  return *this;
}

MeshLoader& ObjLoader::doLoad(const void *data, size_t sz)
{
  m_mesh.m_v.reserve(InitialVertices);
  m_mesh.m_vt.reserve(InitialTexCoords);
  m_mesh.m_vn.reserve(InitialNormals);

  m_mesh.m_tris.reserve(InitialVertices);

  util::splitlines(std::string((const char *)data, sz), [this](const std::string& line) {
    auto it = line.begin();
    while(it != line.end() && isspace(*it)) it++; // skip leading whitespace

    if(it == line.end() || *it == Comment) return; // Line is empty or a comment - skip it
   
    parseLine(it);
  });

  return *this;
}

MeshLoader& ObjLoader::initBuffers(const gx::VertexFormat& fmt, gx::BufferHandle verts)
{
  STUB();

  return *this;
}

MeshLoader& ObjLoader::initBuffers(const gx::VertexFormat& fmt,
  gx::BufferHandle verts, gx::BufferHandle inds)
{
  auto vertex_sz = fmt.vertexByteSize();
  auto index_sz = inds.get<gx::IndexBuffer>().elemSize();

  verts().init(vertex_sz, m_mesh.vertices().size());
  inds().init(index_sz*3 /* each face has 3 vertices */, m_mesh.faces().size());

  return *this;
}

MeshLoader& ObjLoader::doStream(const gx::VertexFormat& fmt, gx::BufferHandle verts)
{
  STUB();

  return *this;
}

MeshLoader& ObjLoader::doStreamIndexed(const gx::VertexFormat& fmt,
  gx::BufferHandle vert_buf, gx::BufferHandle ind_buf)
{
  if(!m_mesh.loaded()) MeshLoader::load();

  initBuffers(fmt, vert_buf, ind_buf);

  auto verts_view = vert_buf().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);
  auto inds_view = ind_buf().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);

  auto verts = verts_view.get<byte>();

  const vec3 *v_src = m_mesh.vertices().data();
  const vec3 *vn_src = m_mesh.normals().data();
  const vec3 *vt_src = m_mesh.texCoords().data();

  auto vertex_stride = fmt.vertexByteSize();
  StridePtr<vec3> v_dst(verts + 0, vertex_stride);
  StridePtr<vec3> vn_dst(verts + sizeof(vec3), vertex_stride);
  StridePtr<vec2> vt_dst(verts + sizeof(vec3) + sizeof(vec3), vertex_stride);

  if(!m_mesh.hasNormals() && !m_mesh.hasTexCoords()) {
    for(size_t i = 0; i < m_mesh.vertices().size(); i++) {
      *v_dst++ = *v_src++;
    }
  } else if(m_mesh.hasNormals() && !m_mesh.hasTexCoords()) {
    for(size_t i = 0; i < m_mesh.vertices().size(); i++) {
      *v_dst++ = *v_src++;
      *vn_dst++ = *vn_src++;
    }
  } else {
    for(size_t i = 0; i < m_mesh.vertices().size(); i++) {
      auto vt = *vt_src++;

      *v_dst++ = *v_src++;
      *vn_dst++ = *vn_src++;
      *vt_dst++ = vt.xy();
    }
  }

  switch(ind_buf.get<gx::IndexBuffer>().elemType()) {
  case gx::u8: {
    u8 *inds = inds_view.get<u8>();
    for(const auto& face : m_mesh.faces()) {
      for(const auto& vert: face) *inds++ = (u8)vert.v;
    }
    break;
  }

  case gx::u16: {
    u16 *inds = inds_view.get<u16>();
    for(const auto& face : m_mesh.faces()) {
      for(const auto& vert : face) *inds++ = (u16)vert.v;
    }
    break;
  }

  case gx::u32: {
    u32 *inds = inds_view.get<u32>();
    for(const auto& face : m_mesh.faces()) {
      for(const auto& vert : face) *inds++ = (u32)vert.v;
    }
    break;
  }

  default: assert(0); // unreachable
  }

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

bool ObjMesh::loaded() const
{
  return !m_tris.empty();
}

bool ObjMesh::hasNormals() const
{
  return !m_vn.empty();
}

bool ObjMesh::hasTexCoords() const
{
  return !m_vt.empty();
}

const std::vector<ObjMesh::Triangle>& ObjMesh::faces() const
{
  return m_tris;
}

}