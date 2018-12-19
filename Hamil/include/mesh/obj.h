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

  // 0-based indices
  struct Vertex {
    uint v  = None;
    uint vt = None;
    uint vn = None;

    // Returns 'true' when:
    //    v == vt == vn  (where v,vn,vt != None)
    // doesn't hold for this vertex
    bool needsNormalization() const
    {
      return (vn != ObjMesh::None && vn != v) ||
        (vt != ObjMesh::None && vt != v) ||
        (vn != ObjMesh::None && vt != ObjMesh::None && vn != vt);
    }
  };

  using Triangle = std::array<Vertex, 3>;

  // Name given in an 'o' command
  const std::string& name() const;

  // Returns 'true' when the mesh
  //   has had it's faces loaded
  bool loaded() const;

  // Returns the offset in the index buffer
  //   emmited by MeshLoader::streamIndexed()
  //   for this mesh expressed in number of
  //   IndexBuffer elements
  size_t offset() const;

  // Returns a list of indices corresponding to
  //   the parent loader's vertices(), normals(),
  //   texCoords() vectors
  const std::vector<Triangle>& faces() const;

  // Returns an AABB of all the vertices
  //   referenced by this mesh
  AABB aabb() const;

private:
  friend class ObjLoader;

  std::string m_name;

  // Defined so that:
  //   - for 'min' any vertex encountered will be
  //     considered "smaller"
  //   - analogously for 'max' any mesh vertex will
  //     be considered "bigger"
  AABB m_aabb = { vec3(INFINITY), vec3(-INFINITY) };

  size_t m_offset;   // See note above offset()
  std::vector<Triangle> m_tris;
};

class ObjLoader : public MeshLoader {
public:
  enum LoadFlags {
    Default = 0,

    // Make sure  that for every triangle
    //     v == vn == vt  (where v,vn,vt != None)
    Normalize = 1<<0,
  };

  ObjLoader& load(const void *data, size_t sz, uint /* LoadFlags */ flags = Default);

  // Returns the number of objects in the *.obj file
  size_t numMeshes() const;

  // DO NOT chain this method through load() as it will
  //    cause a dangling refernce to be returned
  // The ObjLoader object must be kept around in order to 
  //   access the mesh(es)
  //  - Indices are assigned sequentially (from 0)
  //    in the order of appereance in the file
  const ObjMesh& mesh(uint index = 0) const;

  // Same as mesh(uint), except the 'name' string is
  //  used to find the ObjMesh
  //   - Returns 'nullptr' if a mesh with 'name' doesn't exist
  const ObjMesh *mesh(const std::string& name) const;

  // Returns 'true' when the mesh has vertex normals defined
  bool hasNormals() const;
  // Returns 'true' when the mesh has texture coordinates defined
  bool hasTexCoords() const;

  const std::vector<vec3>& vertices() const;
  const std::vector<vec3>& normals() const;
  const std::vector<vec3>& texCoords() const;

protected:
  virtual MeshLoader& doLoad(const void *data, size_t sz);

  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt, gx::BufferHandle verts);
  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds);

  virtual MeshLoader& doStream(const gx::VertexFormat& fmt, gx::BufferHandle verts);
  virtual MeshLoader& doStreamIndexed(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds);

private:
  // Appends a new ObjMesh to 'm_meshes' and returns a
  //   pointer to it (do NOT keep it around as
  //   it can become invalidated after another call
  //   to this method)
  ObjMesh *appendMesh(const std::string& name = "");

  // Processes a single line of the file
  //   - The line must start with an *.obj command
  //     (i.e. caller must take care of skipping
  //     whitespace)
  void parseLine(std::string::const_iterator it);

  // Implements 'o' command
  std::string loadName(std::string::const_iterator it);

  // Helper for load<Vertex,TexCoord,Normal>()
  vec3 loadVec3(std::string::const_iterator it);

  // Implements 'v' command
  vec3 loadVertex(std::string::const_iterator it);
  // Implements 'vt' command
  vec3 loadTexCoord(std::string::const_iterator it);
  // Implements 'vn' command
  vec3 loadNormal(std::string::const_iterator it);

  // Implements 'f' command
  //   - Throws MeshLoader::NonTriangularFaceError when
  //     a face with > 3 vertices is encountered
  ObjMesh::Triangle loadTriangle(std::string::const_iterator it);

  // Ensures that, for every face, v == vn == vt
  //   - Useful to allow the mesh to be used
  //     by OpenGL (where only 1 index must
  //     be used for all vertex attributes)
  //   - 'm_normalized' is NOT checked by
  //     this method
  void normalizeMeshes();

  // Called by normalizeMeshes()
  void normalizeOne(ObjMesh& mesh);

  // After load() this flag will be == false
  //   if normalizeMeshes() needs to be called
  bool m_normalized;

  // Vertex attributes are shared among
  //  all objects in an *.obj file

  std::vector<vec3> m_v;
  std::vector<vec3> m_vt;
  std::vector<vec3> m_vn;

  // Incremented by loadTriangle()
  size_t m_current_offset;

  // List of objects loaded from the file
  std::vector<ObjMesh> m_meshes;
};


}