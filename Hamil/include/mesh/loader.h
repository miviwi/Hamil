#pragma once

#include <common.h>

#include <util/unit.h>
#include <sched/job.h>
#include <gx/vertex.h>
#include <gx/buffer.h>

#include <memory>

namespace mesh {

// TODO:
//   - stream<Indexed>() must somehow be able to understand
//     VertexFormats and unpack the vertices based on them
//   - Maybe split gx related operations into a different class?
class MeshLoader {
public:
  struct Error { };

  struct ParseError : public Error { };
  struct NonTriangularFaceError : Error { };

  using StreamJobPtr = std::unique_ptr<sched::IJob>;

  // Uses the loadParams()
  MeshLoader& load();

  // Stores 'data' pointer for async loading in stream<Indexed>()
  MeshLoader& loadParams(const void *data, size_t sz);

  // Fills 'verts' with the Meshes vertices based on 'fmt'
  //   - The returned StreamJobPtr must be passed directly
  //     to WorkerPool::scheduleJob()
  StreamJobPtr stream(const gx::VertexFormat& fmt, gx::BufferHandle verts);
  // Fills 'verts' with the Meshes vertices (based on 'fmt') and 'inds' with
  //  it's indices (based on gx::IndexBuffer::elemType())
  //   - The returned StreamJobPtr must be passed directly
  //     to WorkerPool::scheduleJob()
  StreamJobPtr streamIndexed(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds);

protected:
  // 'sz' is the length of 'data' in bytes
  virtual MeshLoader& doLoad(const void *data, size_t sz) = 0;

  // Calls Buffer::init() with mesh-dependent parameters
  //   - Stubbed out when the Mesh format is indexed-only
  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt, gx::BufferHandle verts) = 0;
  // Calls Buffer::init() with mesh-depentent parameters
  virtual MeshLoader& initBuffers(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds) = 0;

  // - Calls load() if the mesh hasn't been loaded yet
  // - Stubbed out when the Mesh format is indexed-only
  virtual MeshLoader& doStream(const gx::VertexFormat& fmt,  gx::BufferHandle verts) = 0;
  // - Calls load() when mesh hasn't been loaded yet
  // TODO: for now the format of the vertices is hard-coded into the
  //       streaming logic for praticular Mesh formats
  virtual MeshLoader& doStreamIndexed(const gx::VertexFormat& fmt,
    gx::BufferHandle verts, gx::BufferHandle inds) = 0;

private:
  const void *m_data = nullptr;
  size_t m_sz = ~0ull;
};

}