#include <hm/hamil.h>
#include <hm/world.h>

#include <util/format.h>

#include <cassert>

namespace hm {

World::Ptr p_default_world;

void init()
{
  p_default_world.reset(World::alloc());
}

void finalize()
{
  p_default_world.reset();
}

World& world()
{
  assert(p_default_world && "hm::world() called without hm::init()!");

  return *p_default_world;
}

void frame()
{
}

}
