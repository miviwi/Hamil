#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/prototype.h>

#include <util/passkey.h>
#include <util/smallvector.h>

#include <initializer_list>
#include <array>
#include <tuple>
#include <memory>

namespace hm {

// Forward declarations
class EntityManager;
class EntityQueryParams;

enum class ComponentAccess : unsigned;

class EntityQuery {
public:
  EntityQuery(const EntityQuery&) = delete;
  EntityQuery(EntityQuery&& other) = default;     // TODO: define this explicitly!

//semi-protected:
  using EntityManagerKey = util::PasskeyFor<EntityManager>;

  // The constructors are marked private as creating EntityQueries can be done
  //  only via an EntityManager instance from eg. the default hm::world()
  static EntityQuery empty_query(EntityManagerKey = {});

  //    --------------  EntityManager internal  -----------------
  using ComponentTypeList = std::initializer_list<ComponentProtoId>;
  using ComponentTypeListPtr = std::tuple<const ComponentProtoId *, size_t /* num */>;

  // Append a QueryConditions set allowing AccessNone to Component data 'm_conds'
  EntityQuery& withNoAccess(
      ComponentTypeList all, ComponentTypeList any = {}, ComponentTypeList none = {},
      EntityManagerKey = {}
  );

  // Append a QueryConditions set allowing AccessReadOnly to Component data 'm_conds'
  EntityQuery& withReadOnly(
      ComponentTypeList all, ComponentTypeList any = {}, ComponentTypeList none = {},
      EntityManagerKey = {}
  );

  // Append a QueryConditions set allowing AccessWriteOnly to Component data 'm_conds'
  EntityQuery& withWriteOnly(
      ComponentTypeList all, ComponentTypeList any = {}, ComponentTypeList none = {},
      EntityManagerKey = {}
  );

  // Append a QueryConditions set allowing AccessReadWrite to Component data 'm_conds'
  EntityQuery& withReadWrite(
      ComponentTypeList all, ComponentTypeList any = {}, ComponentTypeList none = {},
      EntityManagerKey = {}
  );

  EntityQuery& withAccessByPtr(
      ComponentAccess access,
      ComponentTypeListPtr all, ComponentTypeListPtr any, ComponentTypeListPtr none,
      EntityManagerKey = {}
  );

  //    ---------------------------------------------------------

protected:
  EntityQuery() = default;

private:
  template <size_t N = 64>
  using ComponentTypeVec = util::SmallVector<ComponentProtoId, N>;

  enum ConditionAccessMode {
    AccessNone,
    AccessReadOnly,
    AccessWriteOnly,
    AccessReadWrite,

    NumAccessModes,
  };

  struct QueryConditions {
    ComponentTypeVec<128> all;
    ComponentTypeVec< 32> any;
    ComponentTypeVec< 32> none;
  };

  using ComponentTypeMap = EntityPrototype::ComponentTypeMap;

  template <ConditionAccessMode AccessMode>
  EntityQuery& withConditions(
      const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none)
  {
    auto conds_for_access = std::addressof(std::get<AccessMode>(m_conds));

    return withConditionsByAccess(all, any, none, conds_for_access);
  }

  EntityQuery& withConditionsByAccess(
      const ComponentTypeListPtr& all, const ComponentTypeListPtr& any, const ComponentTypeListPtr& none,
      ConditionAccessMode access
  );

  std::array<QueryConditions, NumAccessModes> m_conds;

  struct {
    ComponentTypeMap all;
    ComponentTypeMap any;
    ComponentTypeMap none;
  } m_union_op;

  struct {
    ComponentTypeMap read;
    ComponentTypeMap write;
  } m_access_mask;
};

}
