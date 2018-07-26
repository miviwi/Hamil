#include <res/res.h>
#include <res/manager.h>
#include <res/cache.h>
#include <res/loader.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>

#include <win32/file.h>

#include <memory>
#include <regex>

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

}