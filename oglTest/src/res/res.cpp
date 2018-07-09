#include <res/res.h>
#include <res/manager.h>
#include <res/cache.h>
#include <res/loader.h>
#include <res/resource.h>
#include <res/text.h>

#include <win32/file.h>

#include <memory>

namespace res {

ResourceManager::Ptr p_manager;

void init()
{
  p_manager = ResourceManager::Ptr(new ResourceManager(
    { new SimpleFsLoader("C:/00PROJ/oglTest/x64/Debug"), }
  ));
}

void finalize()
{
}

ResourceManager& resource()
{
  return *p_manager;
}

ResourceHandle resource(size_t guid)
{
  return p_manager->handle(guid);
}

void resourcegen(std::vector<std::string> resources)
{
  if(resources.empty()) {
    std::function<void(const std::string&)> enum_resources = [&](const std::string& path) {
      win32::FileQuery q((path + "*").data());

      q.foreach([&](const char *name, win32::FileQuery::Attributes attrs) {
        auto full_name = path + name;

        if(attrs & win32::FileQuery::IsDirectory) {
          enum_resources((full_name + "/").data());
        } else {
          resources.emplace_back(full_name);
        }
      });
    };

    enum_resources("");
  }

  for(const auto& resource : resources) {
    puts(resource.data());
  }
}

}