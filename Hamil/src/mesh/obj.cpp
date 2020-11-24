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

ObjLoader& ObjLoader::load(const void *data, size_t sz, uint flags)
{
  doLoad(data, sz);

  if(flags & Normalize && !m_normalized) normalizeMeshes();

  return *this;
}

size_t ObjLoader::numMeshes() const
{
  return m_meshes.size();
}

MeshLoader& ObjLoader::doLoad(const void *data, size_t sz)
{
  m_normalized = true;

  m_v.reserve(InitialVertices);
  m_vt.reserve(InitialTexCoords);
  m_vn.reserve(InitialNormals);

  m_current_offset = 0;

  appendMesh();  // Make sure back() doesn't get called when m_meshes is empty

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

  // Calculate the cummulative number of
  //   vertices and indices
  size_t num_verts = m_v.size(); // Meshes should be normalized by this point, which implies:
                                 //       m_v.size() == m_vn.size() == m_vt.size()
  size_t num_inds  = 0;
  for(auto& mesh : m_meshes) {
    num_inds  += mesh.faces().size();
  }

  verts().init(vertex_sz, num_verts);
  inds().init(index_sz*3 /* each face has 3 vertices */, num_inds);

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
  if(m_meshes.empty()) {
    MeshLoader::load();

    // Meshes must be normalized so they can be
    //   streamed into the Vertex/IndexBuffers
    if(!m_normalized) normalizeMeshes();
  }

  initBuffers(fmt, vert_buf, ind_buf);

  auto verts_view = vert_buf().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);
  auto inds_view = ind_buf().map(gx::Buffer::Write, gx::Buffer::MapInvalidate);

  auto verts = verts_view.get<byte>();

  const vec3 *v_src  = vertices().data();
  const vec3 *vn_src = normals().data();
  const vec3 *vt_src = texCoords().data();

  auto vertex_stride = fmt.vertexByteSize();
  StridePtr<vec3> v_dst(verts + 0, vertex_stride);
  StridePtr<vec3> vn_dst(verts + sizeof(vec3), vertex_stride);
  StridePtr<vec2> vt_dst(verts + sizeof(vec3) + sizeof(vec3), vertex_stride);

  if(!hasNormals() && !hasTexCoords()) {
    for(size_t i = 0; i < vertices().size(); i++) {
      *v_dst++ = *v_src++;
    }
  } else if(hasNormals() && !hasTexCoords()) {
    for(size_t i = 0; i < vertices().size(); i++) {
      *v_dst++ = *v_src++;
      *vn_dst++ = *vn_src++;
    }
  } else {
    for(size_t i = 0; i < vertices().size(); i++) {
      auto vt = *vt_src++;

      *v_dst++ = *v_src++;
      *vn_dst++ = *vn_src++;
      *vt_dst++ = vt.xy();
    }
  }

  union {
    void *inds;

    u8 *bytes;
    u16 *shorts;
    u32 *ints;
  };
  inds = inds_view.get();

  // Write out all the indices
  for(auto& mesh : m_meshes) {
    for(const auto& face : mesh.faces()) {
      // Loop-invariant code motion should fix this up :)
      switch(ind_buf.get<gx::IndexBuffer>().elemType()) {
      case gx::Type::u8:  for(const auto& vert: face) *bytes++  = (u8)vert.v; break;
      case gx::Type::u16: for(const auto& vert: face) *shorts++ = (u16)vert.v; break;
      case gx::Type::u32: for(const auto& vert: face) *ints++   = (u32)vert.v; break;

      default: assert(0); // unreachable
      }
    }
  }

  return *this;
}

const ObjMesh& ObjLoader::mesh(uint index) const
{
  return m_meshes.at(index);
}

const ObjMesh *ObjLoader::mesh(const std::string& name) const
{
  auto it = std::find_if(m_meshes.begin(), m_meshes.end(), [&](const ObjMesh& mesh) {
    return mesh.name() == name;
  });

  const ObjMesh *ptr = nullptr;
  if(it != m_meshes.end()) { // We've found it
    const auto& mesh = *it;

    ptr = &mesh;
  }

  return ptr;
}

bool ObjLoader::hasNormals() const
{
  return !m_vn.empty();
}

bool ObjLoader::hasTexCoords() const
{
  return !m_vt.empty();
}

const std::vector<vec3>& ObjLoader::vertices() const
{
  return m_v;
}

const std::vector<vec3>& ObjLoader::normals() const
{
  return m_vn;
}

const std::vector<vec3>& ObjLoader::texCoords() const
{
  return m_vt;
}

ObjMesh *ObjLoader::appendMesh(const std::string& name)
{
  auto& mesh = m_meshes.emplace_back();

  mesh.m_name = name;


  mesh.m_tris.reserve(InitialVertices);

  return &mesh;
}

void ObjLoader::parseLine(std::string::const_iterator it)
{
  char keyword = *it;
  it++;

  // Take the current (i.e. most recently added) mesh
  auto& mesh = m_meshes.back();

  switch(keyword) {
  case Vertex:
    if(isspace(*it)) {  // This is a 'v' command
      [[maybe_unused]] auto& aabb = mesh.m_aabb;
      auto vert = loadVertex(it);

      m_v.push_back(vert);
    } else {            // Either a 'vn' or 'vt'
      char ch = *it;
      it++;

      switch(ch) {
      case TexCoord: m_vt.push_back(loadTexCoord(it)); break;
      case Normal:   m_vn.push_back(loadNormal(it)); break;

      default: throw ParseError();
      }
    }
    break;

  case Face:
    mesh.m_tris.push_back(loadTriangle(it));
    break;

  case Object:
  case Group:    // TODO: handle groups differently (?)
    if(mesh.m_tris.empty()) {
      // A mesh is always created (in case the .obj
      //   has no 'o' command), but when objects ARE
      //   defined, calling appendMesh() here would
      //   create an empty one at index 0, so use it
      //   instead of creating a new one
      mesh.m_name = loadName(it);
    } else {
      // When faces have already been appended
      //   to the current mesh - append a new one
      //  - Next time parseLine() is called it
      //    will automatically choose it
      auto new_mesh = appendMesh(loadName(it));

      new_mesh->m_offset = m_current_offset;
    }
    break;

  case Shading: break;   //  Ignore
  case Material: break;  // -- || --
  case UseMtl: break;    // -- || -- 

  case Comment: return;   // Nothing to do

  default: throw ParseError();
  }
}

std::string ObjLoader::loadName(std::string::const_iterator it)
{
  std::string name;

  while(isspace(*it)) {  // Skip whitespace
    if(*it == '\n') throw ParseError();  // A name wasn't specified

    it++;
  }

  // Read the name (this ignores potential
  //   extreneous characters at the end of
  //   the line - oh well)
  while(!isspace(*it)) name.push_back(*it++);

  return name;
}

vec3 ObjLoader::loadVec3(std::string::const_iterator it)
{
  const char *str = &(*it);

  alignas(16) vec3 v;
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

  auto& mesh = m_meshes.back();

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
        it->vt = read_index();
      } else {
        end = (char *)pos;
      }
    }

    if(*end == '/') {
      pos = end+1;
      it->vn = read_index();
    }

    // Done appending this vertex
    m_current_offset++;

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

    // Any un-normalized vertex causes all the meshes
    //   to have to be normalized
    if(v.needsNormalization()) m_normalized = false;

    // Recompute the AABB (for the current ObjMesh)
    auto vert = m_v[v.v];
    mesh.m_aabb = {
      vec3::min(mesh.m_aabb.min, vert), vec3::max(mesh.m_aabb.max, vert)
    };
  }

  return triangle;
}

void ObjLoader::normalizeMeshes()
{
  if(!hasNormals() && !hasTexCoords()) return;  // Nothing to do

  for(auto& mesh : m_meshes) normalizeOne(mesh);
}

void ObjLoader::normalizeOne(ObjMesh& mesh)
{
  auto num_verts = std::max({ m_v.size(), m_vn.size(), m_vt.size() });

  bool has_normals = hasNormals(),
    has_texcoords = hasTexCoords();

  // Make sure all attribute array elements have
  //   corresponding ones in all other arrays - only
  //   when a given attribute is present, though
  if(m_v.size() < num_verts) m_v.resize(num_verts);
  if(has_normals)   m_vn.resize(num_verts);
  if(has_texcoords) m_vt.resize(num_verts);

  // When an unnormalized Vertex is found,
  //   a new v,vn,vt is appended to the attribute
  //   vectors and the Vertex' indices are updated
  for(auto& face : mesh.m_tris) {
    for(auto& vert : face) {
      if(!vert.needsNormalization()) continue;

      if(has_normals) {
        auto new_index = (uint)m_vn.size();
        auto vn = m_vn[vert.vn];

        vert.vn = new_index;
        m_vn.emplace_back(vn);
      }

      if(has_texcoords) {
        auto new_index = (uint)m_vt.size();
        auto vt = m_vt[vert.vt];

        vert.vt = new_index;
        m_vt.emplace_back(vt);
      }

      // Positions are always present
      auto new_index = (uint)m_v.size();
      auto v  = m_v[vert.v];

      vert.v  = new_index;
      m_v.emplace_back(v);
    }
  }
}

const std::string& ObjMesh::name() const
{
  return m_name;
}

bool ObjMesh::loaded() const
{
  return !m_tris.empty();
}

size_t ObjMesh::offset() const
{
  return m_offset;
}

const std::vector<ObjMesh::Triangle>& ObjMesh::faces() const
{
  return m_tris;
}

AABB ObjMesh::aabb() const
{
  return m_aabb;
}

}
