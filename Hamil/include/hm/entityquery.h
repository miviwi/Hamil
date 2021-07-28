#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/prototype.h>

#include <util/passkey.h>
#include <util/smallvector.h>

#include <initializer_list>
#include <array>
#include <tuple>
#include <optional>
#include <memory>
#include <functional>

namespace hm {

// Forward declarations
class IEntityManager;
class EntityManager;
class EntityQueryParams;
class PrototypeChunk;

enum class ComponentAccess : unsigned;
// --------------------

namespace detail {

struct CollectedChunkList {
  enum { InvalidVersion = -1 };

  static CollectedChunkList null_chunk();
  static CollectedChunkList chunk(u32 protocid, PrototypeChunk *chunks, int version = InvalidVersion);

  int collected_version;
  u32 proto_cacheid;
  PrototypeChunk *chunks_list;
};

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
  EntityQuery(EntityQuery&& other) = default;     // TODO: define this explicitly!

  using ComponentIterFn = std::function<void(ComponentProtoId, UnionOp, ComponentAccess)>;

  const EntityQuery& foreachComponent(ComponentIterFn&& fn) const;
  const EntityQuery& foreachComponentGroupedByAccess(ComponentIterFn&& fn) const;

  // Populate internal list of matching entity PrototypeChunks
  EntityQuery& collectEntities();

  void dbg_PrintQueryComponents(bool group_by_access = true) const;

//semi-protected:
  using EntityManagerKey = util::PasskeyFor<EntityManager>;

  // The constructors are marked private as creating EntityQueries can be done
  //  only via an EntityManager instance from eg. the default hm::world()
  static EntityQuery empty_query(IEntityManager *entity_man, EntityManagerKey = {});

  //    --------------  EntityManager internal  -----------------
  using ComponentTypeListPtr = std::tuple<const ComponentProtoId *, size_t /* num */>;

  EntityQuery& allWithAccess(ComponentAccess access, ComponentTypeListPtr all, EntityManagerKey = {});
  EntityQuery& anyWithAccess(ComponentAccess access, ComponentTypeListPtr any, EntityManagerKey = {});
  EntityQuery& noneWithAccess(ComponentAccess access, ComponentTypeListPtr none, EntityManagerKey = {});
  //    ---------------------------------------------------------

protected:
  EntityQuery(IEntityManager *entity_man);

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

  IEntityManager *m_entity = nullptr;

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
