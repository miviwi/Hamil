#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/prototype.h>
#include <hm/chunkhandle.h>
#include <hm/queryparams.h>

#include <util/passkey.h>
#include <util/smallvector.h>

#include <initializer_list>
#include <array>
#include <tuple>
#include <optional>
#include <utility>
#include <memory>
#include <functional>

namespace hm {

// Forward declarations
class IEntityManager;
class EntityManager;
class EntityPrototypeCache;
class PrototypeChunk;
class PrototypeChunkHandle;

enum class ComponentAccess : unsigned;
// --------------------

namespace detail {

struct CollectedChunkList {
  enum { InvalidVersion = -1 };

  using HandleList = util::SmallVector<PrototypeChunkHandle, 56 /* so sizeof(CollectedChunkList)==64 */> ;

  static CollectedChunkList create_null();
  static CollectedChunkList create_empty(u32 protocid, int version = InvalidVersion);

  int collected_version;
  u32 proto_cacheid;
  HandleList chunks_list;
};
static_assert(sizeof(CollectedChunkList) == 64, "fix CollectedChunkList size");

}

class EntityQuery {
public:
  enum UnionOp {
    UnionInvalid,
    UnionAll,
    UnionAny,
    UnionNone,
  };

  EntityQuery(const EntityQuery&) = delete;
  EntityQuery(EntityQuery&& other) noexcept;

  EntityQuery& operator=(EntityQuery&& other) noexcept;

  using ComponentIterFn = std::function<void(ComponentProtoId, UnionOp, ComponentAccess)>;

  const EntityQuery& foreachComponent(ComponentIterFn&& fn) const;
  const EntityQuery& foreachComponentGroupedByAccess(ComponentIterFn&& fn) const;

  // Populate internal list of matching entity PrototypeChunks
  EntityQuery& collectEntities();

  using EntityIterFn = std::function<void(Entity, u32 /* alloc_id */)>;
  EntityQuery& foreachEntity(EntityIterFn&& fn);

  void swap(EntityQuery& other) noexcept;

  void dbg_PrintQueryComponents(bool group_by_access = true) const;
  void dbg_PrintCollectedChunks() const;

//semi-protected:
  using EntityManagerKey = util::PasskeyFor<EntityManager>;

  // The constructors are marked private as creating EntityQueries can be done
  //  only via an EntityManager instance from eg. the default hm::world()
  static EntityQuery empty_query(
      const IEntityManager *entity_man, EntityManagerKey = {}
  );

  // 'params' should point to the EntityQueryParams structure used to set up this query
  //  - MUST be called before collectEntities()
  EntityQuery& injectCreationParams(const IEntityQueryParams *p, EntityManagerKey = {}) &;
  EntityQuery&& injectCreationParams(const IEntityQueryParams *p, EntityManagerKey = {}) &&;

  //    --------------  EntityManager internal  -----------------
  using ComponentTypeListPtr = std::tuple<const ComponentProtoId *, size_t /* num */>;

  EntityQuery& allWithAccess(ComponentAccess access, ComponentTypeListPtr all, EntityManagerKey = {});
  EntityQuery& anyWithAccess(ComponentAccess access, ComponentTypeListPtr any, EntityManagerKey = {});
  EntityQuery& noneWithAccess(ComponentAccess access, ComponentTypeListPtr none, EntityManagerKey = {});
  //    ---------------------------------------------------------

protected:
  EntityQuery(const IEntityManager *entity_man = nullptr);

private:
  enum ConditionAccessMode {
    AccessNone,
    AccessReadOnly,
    AccessWriteOnly,
    AccessReadWrite,

    NumAccessModes,

    AccessInvalid = -1,
  };

  using ComponentTypeMap = EntityPrototype::ComponentTypeMap;

  using CollectedChunkList = detail::CollectedChunkList;
  static constexpr size_t CollectedChunksInlineSize = 256;

  using CollectedChunks = util::SmallVector<CollectedChunkList, CollectedChunksInlineSize>;

  template <ConditionAccessMode AccessMode>
  EntityQuery& withConditions(
      const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none)
  {
    return withConditionsByAccess(all, any, none, AccessMode);
  }

  EntityQuery& withConditionsByAccess(
      const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none,
      ConditionAccessMode access
  );

  ComponentTypeMap jointComponentTypeMask() const;

  static ConditionAccessMode to_access_mode(ComponentAccess access);
  static ComponentAccess from_access_mode(ConditionAccessMode access_mode);

  static ComponentProtoId find_and_clear_lsb(u64 *components);

  template <typename Fn>
  static void foreach_bit_qword(u64 components, u64 base, Fn&& fn)
  {
    while(components) {
      auto next = find_and_clear_lsb(&components);

      fn(base + next);
    }
  }

  void assertComponentNotAdded(ComponentProtoId component);

  std::unique_ptr<const IEntityQueryParams>  m_params;
  const IEntityManager                      *m_entity;

  struct {
    ComponentTypeMap all;
    ComponentTypeMap any;
    ComponentTypeMap none;
  } m_union_op;

  struct {
    ComponentTypeMap read;
    ComponentTypeMap write;
  } m_access_mask;

  std::optional<CollectedChunks> m_chunks = std::nullopt;

};

}

namespace std {

using EntityQuery = hm::EntityQuery;

template <>
inline void swap<EntityQuery>(EntityQuery& lhs, EntityQuery& rhs) noexcept
{
  lhs.swap(rhs);
}

}

