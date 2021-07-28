#include <hm/world.h>
#include <hm/entityman.h>
#include <hm/chunkman.h>

#include <new>
#include <memory>

#include <cassert>

namespace hm {

struct WorldData {
  IEntityManager::Ptr entities;

  // EntityManager dependencies:
  ChunkManager::Ptr entity_chunks;
};

World *World::alloc()
{
  return WithPolymorphicStorage::alloc<World, WorldData>();
}

void World::destroy(World *world)
{
  WithPolymorphicStorage::destroy<WorldData>(world);
}

World::World()
{
  new(storage<WorldData>()) WorldData();
}

World& World::createEmpty()
{
  assert(!storage<WorldData>()->entities && "this World has been created already!");

  auto d = storage<WorldData>();

  d->entities = create_entity_manager();

  // Create and inject EntityManager dependencies:
  //   - ChunkManager
  d->entity_chunks = create_chunk_manager();
  d->entities->injectChunkManager(d->entity_chunks.get());

  return *this;
}

IEntityManager& World::entities()
{
  assert(storage<WorldData>()->entities && "World::entities() called before create!");

  return *storage<WorldData>()->entities;
}

std::weak_ptr<IEntityManager> World::entitiesRef()
{
  assert(storage<WorldData>()->entities && "World::entitiesRef() called before create!");

  return storage<WorldData>()->entities;
}

}
