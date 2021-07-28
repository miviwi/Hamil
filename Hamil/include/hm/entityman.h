#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>

#include <memory>
#include <string>

namespace hm {

// Forward declarations
class EntityPrototype;
class EntityPrototypeCache;
class CachedPrototype;
class ChunkManager;
class BoundPrototypeChunk;
class EntityQuery;
class IEntityQueryParams;
class EntityQueryParams;

class IEntityManager {
public:
  using Ptr = std::shared_ptr<IEntityManager>;

  virtual ~IEntityManager() = default;

  // Stores a reference to 'chunk_man' internally, which will be used upon
  //   future calls to createEntity(), findEntity(), destroyEntity(), ...
  //  - The reference is a WEAK reference, which means not only is the caller
  //    responsible for destroying the ChunkManager but also ensuring that
  //    it's lifetime doesn't expire before the EntityManager's
  //  - Reassigning via this method is NOT allowed as there is a lot of
  //    internal state which gets entangled with 'chunk_man'
  virtual IEntityManager& injectChunkManager(ChunkManager *chunk_man) = 0;

  // Returns the ChunkManager instance injected via injectChunkManager()/nullptr
  virtual ChunkManager *chunkManager() = 0;

  // Returns a handle to an EntityPrototype (a 'CachedPrototype') which
  //   includes exactly the components specified by 'proto' i.e.
  //      -  cached.components() == proto,   where 'cached' is the return value
  //  - Calls EntityPrototypeCache::fill() on a cache internal to
  //     this IEntityManager on first invocation (when the cache is
  //     cold) for a given EntityPrototype and yields it's return value
  //  - Further calls with matching 'proto' return cached object
  virtual CachedPrototype prototype(const EntityPrototype& proto) = 0;

  virtual const EntityPrototypeCache& prototypeCache() const = 0;

  //  - 'proto' must've been created via a call to THIS IEntityManager's
  //     prototype() method, otherwise expect UB
  virtual Entity createEntity(CachedPrototype proto) = 0;
  virtual Entity findEntity(const std::string& name) = 0;
  virtual void destroyEntity(EntityId id) = 0;

  virtual bool alive(EntityId id) = 0;

  virtual EntityQuery createEntityQuery(const IEntityQueryParams *params) = 0;

  virtual BoundPrototypeChunk prototypeBoundChunk(const CachedPrototype& proto, u32 idx) = 0;
};

IEntityManager::Ptr create_entity_manager();

}
