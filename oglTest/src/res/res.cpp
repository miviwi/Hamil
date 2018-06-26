#include <res/res.h>
#include <res/manager.h>

#include <memory>

namespace res {

std::unique_ptr<ResourceManager> p_manager;

void init()
{
  p_manager = std::make_unique<ResourceManager>();
}

void finalize()
{
}

}