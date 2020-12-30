#pragma once

#include <hm/hamil.h>

#include <util/polystorage.h>

#include <memory>

namespace hm {

// Forward declarations
class IEntityManager;

// PIMPL data
struct WorldData;

class World : WithPolymorphicStorage<> {
public:
  using Ptr = std::unique_ptr<World>;

  static World *alloc();
  static void destroy(World *world);

  World();
  World(const World&) = delete;

  // - Calling this method more than once is forbidden
  World& createEmpty();

  // Calling this without initializing (for ex. via createEmpty() call)
  //   the World beforehand is UB!
  IEntityManager& entities();

};

}
