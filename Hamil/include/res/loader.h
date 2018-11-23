#pragma once

#include <common.h>
#include <res/resource.h>

#include <string>
#include <unordered_map>
#include <tuple>
#include <memory>

namespace win32 {
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

class ResourceLoader {
public:
  using Ptr = std::unique_ptr<ResourceLoader>;

  struct Error { };

  struct InvalidResourceError : public Error {
    const Resource::Id id;
    InvalidResourceError(Resource::Id id_) :
      id(id_)
    { }
  };

  struct IOError : public Error {
    const std::string file;
    IOError(const std::string& file_) :
      file(file_)
    { }
  };

  virtual Resource::Ptr load(Resource::Id id, LoadFlags flags) = 0;
};

// SimpleFsLoader:
//   - loads assets from the specified folder
//   - only support synchronous right-now loading (LoadDefault)
//   - parses the resource-descriptor twice (all of it is slow anyways...)
class SimpleFsLoader : public ResourceLoader {
public:
  SimpleFsLoader(const char *base_path);

  virtual Resource::Ptr load(Resource::Id id, LoadFlags flags);

  using LoaderFn = Resource::Ptr (SimpleFsLoader::*)(Resource::Id, const yaml::Document&);
  Resource::Ptr loadText(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadShader(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadImage(Resource::Id id, const yaml::Document& meta);
  Resource::Ptr loadMesh(Resource::Id id, const yaml::Document& meta);

private:
  void enumAvailable(std::string path);
  Resource::Id enumOne(const char *meta_file, size_t sz, const char *full_path = "");

  using NamePathTuple         = std::tuple<const yaml::Scalar* /* name */, const yaml::Scalar* /* path */>;
  using NamePathLocationTuple = std::tuple<
    const yaml::Scalar* /* name */, const yaml::Scalar* /* path */, const yaml::Scalar* /* location */
  >;

  // Does NOT perform validation!
  static NamePathTuple name_path(const yaml::Document& meta);
  static NamePathLocationTuple name_path_location(const yaml::Document& meta);

  std::string m_path;
  std::unordered_map<Resource::Id, std::vector<char> /* meta_file */>  m_available;
};

}