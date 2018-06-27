#include <res/res.h>
#include <res/manager.h>
#include <res/cache.h>
#include <res/loader.h>
#include <res/resource.h>
#include <res/text.h>

#include <memory>

namespace res {

ResourceManager::Ptr p_manager;

void init()
{
  p_manager = ResourceManager::Ptr(new ResourceManager(
    { TextResource::tag(), },
    { new SimpleFsLoader("C:/00PROJ/oglTest/x64/Debug"), }
  ));
}

void finalize()
{
}

}