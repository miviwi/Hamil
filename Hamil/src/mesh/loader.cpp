#include <mesh/loader.h>

namespace mesh {

MeshLoader& MeshLoader::loadParams(const void *data, size_t sz)
{
  m_data = data;
  m_sz = sz;

  return *this;
}

MeshLoader& MeshLoader::load()
{
  if(!m_data) throw Error();

  return doLoad(m_data, m_sz);
}

MeshLoader::StreamJobPtr MeshLoader::stream(const gx::VertexFormat& fmt, gx::BufferHandle verts)
{
  return StreamJobPtr(new sched::Job<Unit, gx::VertexFormat, gx::BufferHandle>(
    sched::create_job([this](gx::VertexFormat fmt, gx::BufferHandle verts) -> Unit {
      doStream(fmt, verts);
      if(m_on_loaded) m_on_loaded(*this);

      return {};
    }, fmt, verts)
  ));
}

MeshLoader::StreamJobPtr MeshLoader::streamIndexed(const gx::VertexFormat& fmt,
  gx::BufferHandle verts, gx::BufferHandle inds)
{
  return StreamJobPtr(new sched::Job<Unit, gx::VertexFormat, gx::BufferHandle, gx::BufferHandle>(
    sched::create_job([this](gx::VertexFormat fmt, gx::BufferHandle verts, gx::BufferHandle inds) -> Unit {
      doStreamIndexed(fmt, verts, inds);
      if(m_on_loaded) m_on_loaded(*this);

      return {};
    }, fmt, verts, inds)
  ));
}

MeshLoader& MeshLoader::onLoaded(OnLoadedFn fn)
{
  m_on_loaded = fn;

  return *this;
}

}