#pragma once

#include <hm/hamil.h>

#include <util/smallvector.h>

#include <type_traits>
#include <array>

namespace hm {

// Forward declarations
class EntityManager;

enum class ComponentAccess : unsigned {
  None,
  ReadOnly, WriteOnly,
  ReadWrite,

  NumAccessTypes,
};

template <typename Fn>
inline void foreach_component_access(Fn&& fn)
{
  static_assert(std::is_invocable_v<Fn, ComponentAccess>,
      "'fn' expected to be callable with signature void(ComponentAccess)");

  for(unsigned idx = 0; idx < (unsigned)ComponentAccess::NumAccessTypes; idx++) {
    auto access = (ComponentAccess)idx;

    fn(access);
  }
}

class IEntityQueryParams {
public:
  struct ComponentGroupMeta {
    unsigned offset, length;
  };

  enum QueryKind : unsigned {
    QueryKindInvalid,

    AllOf, AnyOf, NoneOf,
  };

  struct RequestedComponent {
    QueryKind kind;
    ComponentProtoId component;
  };

  virtual ~IEntityQueryParams() = default;

  virtual unsigned numQueryComponents() const = 0;

  // Returns a pair of numbers called the - 'offset' and 'length', which describe
  //   the range [offset; offset+length) - left inclusive, right exclusive; all
  //   the included integers correspond to possible indices for componentByIndex()
  //   calls, which will yield components with ComponentAccess 'access' requested
  virtual ComponentGroupMeta componentsForAccess(ComponentAccess access) const = 0;

  // Returns a ComponentProtoId for a component to be requested by the query,
  //   as well as whether to require it, include optionally, or exclude the
  //   whole entity from the query when it's present
  //  - 'index' must fall in the range [0; numQueryComponents()-1]
  virtual RequestedComponent componentByIndex(unsigned index) const = 0;

  friend EntityManager;   // Examined in EntityManager::createEntityQuery() to build
                          //   the resulting EntityQuery object

protected:
  IEntityQueryParams() = default;   // Disallow direct instantiation
};

class EntityQueryParams final : IEntityQueryParams {
public:
  static EntityQueryParams create_empty();
  static EntityQueryParams create_dbg();

  virtual ~EntityQueryParams() = default;

private:
  static constexpr auto NumGroupOffsets = (size_t)ComponentAccess::NumAccessTypes;
  static constexpr unsigned InvalidGroupOffset = ~0u;

  EntityQueryParams();

  virtual unsigned numQueryComponents() const final;
  virtual ComponentGroupMeta componentsForAccess(ComponentAccess access) const final;
  virtual RequestedComponent componentByIndex(unsigned index) const final;

  util::SmallVector<RequestedComponent, 256> m_req;

  // Stores indices of the elements 1-past the last RequestedComponent of the groups
  std::array<unsigned, NumGroupOffsets> m_group_offsets;

};

}
