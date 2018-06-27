#pragma once

#include <common.h>
#include <res/resource.h>

#include <string>
#include <unordered_map>
#include <memory>

namespace win32 {
class FileQuery;
}

namespace res {

enum LoadFlags {
  LoadDefault, LoadStatic, Precache
};

class ResourceLoader {
public:
  using Ptr = std::unique_ptr<ResourceLoader>;

  virtual Resource::Ptr load(Resource::Tag tag, Resource::Id id, LoadFlags flags) = 0;
};

// SimpleFsLoader:
//   - loads assets from the specified folder
//   - only support synchronous right-now loading (LoadDefault)
class SimpleFsLoader : public ResourceLoader {
public:
  SimpleFsLoader(const char *base_path);

  virtual Resource::Ptr load(Resource::Tag tag, Resource::Id id, LoadFlags flags);

private:
  void enumAvailable(std::string path);
  Resource::Id enumOne(const char *meta_file, size_t sz, const char *full_path = "");

  std::unordered_map<Resource::Id, std::string /* file_path */>  m_available;
};

}