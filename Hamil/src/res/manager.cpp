#include <res/manager.h>
#include <res/text.h>
#include <res/shader.h>
#include <res/image.h>
#include <res/mesh.h>
#include <res/lut.h>

#include <util/hash.h>
#include <yaml/node.h>

#include <cstring>
#include <map>
#include <utility>

namespace res {

ResourceManager::ResourceManager(std::initializer_list<ResourceLoader *> loader_chain) :
  m_caches({ { ResourceCache::Generic }, { ResourceCache::Static } }),
  m_loader_chain(loader_chain.size()),
  m_io_workers(NumIOWorkers)
{
  // Kick off the IO workers right away so loaders
  //   can utilize IORequests during init()
  m_io_workers.kickWorkers("ResourceManager_IOWorker");

  // Populate the loader chain in reverse to allow
  //   listing the loaders in a natural order (the last loader
  //   in the std::initializer_list has the highest priority)
  size_t i = loader_chain.size()-1;
  for(const auto& loader : loader_chain) {
    m_loader_chain[i] = ResourceLoader::Ptr(loader->init(this));
    i--;
  }
}

Resource::Id ResourceManager::guid(Resource::Tag tag,
  const std::string& name, const std::string& path)
{
  size_t hash = 0;
  util::hash_combine<Resource::Tag::Hash>(hash, tag);
  util::hash_combine<util::XXHash<std::string>>(hash, name);
  util::hash_combine<util::XXHash<std::string>>(hash, path == "/" ? "" : path);

  return hash;
}

ResourceHandle ResourceManager::load(Resource::Id id, LoadFlags flags)
{
  auto& cache = getCache(flags);
  if(auto r = cache.probe(id)) return *r;

  for(auto& loader : m_loader_chain) {
    if(auto r = loader->load(id, flags)) return cache.fill(r);
  }
  throw Error(); // if no loader manages to locate the resource - throw :(

  return ResourceHandle(); // unreachable
}

ResourceHandle ResourceManager::handle(Resource::Id id)
{
  for(auto& cache : m_caches) {
    if(auto r = cache.probe(id)) return r.value();
  }
  throw NoSuchResourceError(id);

  return ResourceHandle(); // unreachable
}

ResourceManager& ResourceManager::requestIo(const IORequest::Ptr& req)
{
  req->m_id = m_io_workers.scheduleJob(req->job());

  return *this;
}

IOBuffer& ResourceManager::waitIo(const IORequest::Ptr& req)
{
  assert(req->m_id != sched::WorkerPool::InvalidJob &&
    "Attempted to wait for an invalid IORequest!");

  m_io_workers.waitJob(req->m_id);

  return req->result();
}

ResourceManager& ResourceManager::waitIoIdle()
{
  m_io_workers.waitWorkersIdle();

  return *this;
}

IOBuffer ResourceManager::mapLocation(const yaml::Scalar *location,
  size_t offset, size_t sz)
{
  IOBuffer view(nullptr, 0);

  if(location->tag().value() == "!file") {
    win32::File f(location->str(), win32::File::Read, win32::File::OpenExisting);
    size_t map_sz = sz ? sz : f.size();

    return IOBuffer(f.map(win32::File::ProtectRead, offset, map_sz));
  }

  return view;
}

static const std::map<std::string, res::Resource::Tag> p_tags = {
  { Text::tag().get(),   Text::tag()   },
  { Shader::tag().get(), Shader::tag() },
  { Image::tag().get(),  Image::tag()  },
  { Mesh::tag().get(),   Mesh::tag()   },
  { LookupTable::tag().get(), LookupTable::tag() },
};

std::optional<Resource::Tag> ResourceManager::make_tag(const char *tag)
{
  auto it = p_tags.find(tag);
  return it != p_tags.end() ? std::optional<Resource::Tag>(it->second) : std::nullopt;
}

ResourceCache& ResourceManager::getCache(LoadFlags flags)
{
  size_t idx = flags & LoadStatic ? ResourceCache::Static : ResourceCache::Generic;

  return m_caches[idx];
}

}