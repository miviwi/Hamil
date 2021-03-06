#pragma once

#include <common.h>
#include <res/resource.h>
#include <res/io.h>
#include <os/error.h>
#include <os/mutex.h>

#include <string>
#include <unordered_map>
#include <tuple>
#include <memory>

namespace os {
class FileQuery;
}

namespace yaml {
class Document;
class Scalar;
}

namespace res {

enum LoadFlags : int {
  LoadDefault = 0,        // Load the resource immediately
  LoadStatic  = 1<<0,     // Never purge the resource from the cache
  Precache    = 1<<1,     // Load the recource asynchronously, 
                          //   check Resource::loaded() to know when loading is done
};

class ResourceManager;

class ResourceLoader {
public:
  using Ptr = std::unique_ptr<ResourceLoader>;

  virtual ~ResourceLoader() = default;

  struct InvalidResourceError final : public os::Error {
    const Resource::Id id;
    InvalidResourceError(Resource::Id id_) :
      id(id_)
    { }
  };

  struct IOError final : public os::Error {
    const std::string file;
    IOError(const std::string& file_) :
      file(file_)
    { }
  };

  Resource::Ptr load(Resource::Id id, LoadFlags flags);

  ResourceManager& manager();

protected:
  ResourceLoader *init(ResourceManager *manager);

  virtual void doInit() = 0;
  virtual Resource::Ptr doLoad(Resource::Id id, LoadFlags flags) = 0;

private:
  friend ResourceManager;

  ResourceManager *m_man;
};

// SimpleFsLoader:
//   - loads assets from the specified folder
//   - only support synchronous right-now loading (LoadDefault)
//   - parses the resource-descriptor twice (all of it is slow anyways...)
class SimpleFsLoader : public ResourceLoader {
public:
  SimpleFsLoader(const char *base_path);

protected:
  virtual void doInit();
  virtual Resource::Ptr doLoad(Resource::Id id, LoadFlags flags);

private:
  void enumAvailable(std::string path);

  void metaIoCompleted(std::string full_path, IORequest& req);
  static Resource::Id enumOne(const char *meta_file, size_t sz, const char *full_path = "");

  using NamePathTuple = std::tuple<
    const yaml::Scalar* /* name */, const yaml::Scalar* /* path */
  >;
  using NamePathLocationTuple = std::tuple<
    const yaml::Scalar* /* name */, const yaml::Scalar* /* path */, const yaml::Scalar* /* location */
  >;

  // Does NOT perform validation!
  static NamePathTuple name_path(const yaml::Document& meta);
  static NamePathLocationTuple name_path_location(const yaml::Document& meta);

  using LoaderFn = Resource::Ptr(SimpleFsLoader::*)(Resource::Id, const yaml::Document&);

  static const std::unordered_map<Resource::Tag, LoaderFn> loader_fns;

  Resource::Ptr loadText(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadShader(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadImage(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadTexture(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadMesh(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadLUT(Resource::Id id, const yaml::Document& meta);

  std::string m_path;

  // enumAvailable() kicks off IORequests which after completion
  //   call metaIoCompleted() which inserts into m_available. Because
  //   it can be called on multiple threads simultaneously a Mutex
  //   is needed
  os::Mutex::Ptr m_available_mutex;
  std::unordered_map<Resource::Id, IOBuffer /* meta_file */>  m_available;

  std::vector<IORequest::Ptr> m_io_reqs;   // Declared here to avoid use-after-free
                                           //   bugs caused by reference invalidation
                                           //   (see enumAvailable() for details)
};

}
