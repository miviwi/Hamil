#include <res/res.h>
#include <res/manager.h>
#include <res/cache.h>
#include <res/loader.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>

#include <win32/file.h>
#include <util/staticstring.h>

#include <memory>
#include <regex>

namespace res {

ResourceManager::Ptr p_manager;

void init()
{
  p_manager = ResourceManager::Ptr(new ResourceManager(
    { new SimpleFsLoader(__PROJECT_DIR), }
  ));
}

void finalize()
{
  p_manager.reset();
}

ResourceManager& resource()
{
  return *p_manager;
}

ResourceManager& load(std::initializer_list<size_t> ids)
{
  for(auto id : ids) {
    p_manager->load(id);
  }

  return *p_manager;
}

ResourceManager& load(const size_t *ids, size_t sz)
{
  for(size_t i = 0; i < sz; i++) {
    auto id = ids[i];
    p_manager->load(id);
  }

  return *p_manager;
}

ResourceHandle resource(size_t guid)
{
  return p_manager->handle(guid);
}

}