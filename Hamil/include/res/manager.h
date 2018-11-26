#pragma once

#include <common.h>
#include <res/resource.h>
#include <res/io.h>
#include <res/loader.h>
#include <res/cache.h>

#include <sched/pool.h>

#include <string>
#include <vector>
#include <initializer_list>
#include <memory>
#include <optional>
#include <type_traits>

namespace win32 {
class FileView;
}

namespace yaml {
class Scalar;
}

namespace res {

class ResourceManager {
public:
  using Ptr = std::unique_ptr<ResourceManager>;

  enum {
    NumIOWorkers = 2,
  };

  struct Error { };

  struct NoSuchResourceError : public Error {
    const Resource::Id id;
    NoSuchResourceError(Resource::Id id_) :
      id(id_)
    { }
  };

  ResourceManager(std::initializer_list<ResourceLoader *> loader_chain);

  template <typename T>
  Resource::Id guid(const std::string& name, const std::string& path)
  {
    Resource::is_resource<T>();

    return guid(T::tag(), name, path);
  }
  Resource::Id guid(Resource::Tag tag, const std::string& name, const std::string& path) const;

  ResourceHandle load(Resource::Id id, LoadFlags flags = LoadDefault);
  // Gets handle to an already loaded resource
  //   - throws NoSuchResourceError when it can't be found
  ResourceHandle handle(Resource::Id id);

  // Kicks off an IORequest
  ResourceManager& requestIo(const IORequest::Ptr& req);
  // Blocks until 'req' completes and returns it's result
  IOBuffer& waitIo(const IORequest::Ptr& req);

  // - Returns an IOBuffer backed by a FileView when the location is a '!file'
  IOBuffer mapLocation(const yaml::Scalar *location, size_t offset = 0, size_t sz = 0);

  static std::optional<Resource::Tag> make_tag(const char *tag);

private:
  ResourceCache& getCache(LoadFlags flags);

  std::vector<ResourceCache> m_caches;
  std::vector<ResourceLoader::Ptr> m_loader_chain;

  sched::WorkerPool m_io_workers;
};

}