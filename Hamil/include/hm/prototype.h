#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/component.h>

#include <util/fixedbitvector.h>
#include <util/hashindex.h>

#include <initializer_list>
#include <type_traits>
#include <functional>
#include <tuple>

namespace hm {

class EntityPrototype {
public:
  using ComponentTypeMap = util::FixedBitVector<NumComponentProtoIdBits>;

  using Hash = util::HashIndex::Key;

  static Hash hash_type_map(const ComponentTypeMap& components);

  EntityPrototype() = default;
  EntityPrototype(std::initializer_list<ComponentProtoId> components);

  bool equal(const ComponentTypeMap& components) const;
  bool equal(const EntityPrototype& other) const;

  // Returns 'true' if this EntityPrototype includes
  //   at LEAST all the Components included in other
  //   (i.e. returns 'true' if this EntityPrototype
  //   is a superset of 'other')
  bool includes(const EntityPrototype& other) const;

  bool includes(ComponentProtoId component) const;

  const ComponentTypeMap& components() const;

  // Returns the number of UNIQUE Components included in this
  //   prototype's ComponentTypeMap
  size_t numProtoComponents() const;

  size_t chunkCapacity() const;

  // Returns the number of bytes needed to store
  //   a single instance of all components included
  //   in this EntityPrototype
  //  XXX: when a prototype has no Components the
  //       returned value == 0, which makes some
  //       kind of sense and helps in
  //    componentDataOffsetInAoSEntity()
  //       implementation, but maybe this should
  //       be forbidden?
  size_t componentDataSizePerEntity() const;

  // Returns a byte-offset into a struct composed of this
  //   prototype's components (ordered according the values
  //   of ComponentProtoIds, ascending; packed tightly),
  //   where 'component' would be placed
  //  - calling this method when !EntityPrototype::includes(component)
  //    is forbidden!
  //  ex.
  //    assume: sizeof(GameObject)==32, sizeof(Transform)==64, sizeof(Light)==36
  //    let: p = EntityPrototype{ GameObject, Transform, Light }
  //   =>
  //     p.componentDataOffsetInAoSEntity(GameObject) == 0
  //     p.componentDataOffsetInAoSEntity(Light) == 128
  size_t componentDataOffsetInAoSEntity(ComponentProtoId component) const;

  size_t componentDataOffsetInSoAEntityChunk(ComponentProtoId component) const;

  // Returns an integer which is guanarteed to be unique
  //   for EntityPrototypes where:
  //         proto_a.equal(proto_b)
  Hash hash() const;

  template <typename Fn>
  const EntityPrototype& foreachProtoId(Fn&& fn) const
  {
    static_assert(std::is_convertible_v<Fn, std::function<void(ComponentProtoId)>>);

    foreach_bit_qword(m_components.bits.lo, 0, fn);
    foreach_bit_qword(m_components.bits.hi, 64, fn);

    return *this;
  }

  // Returns a new EntityPrototype which includes all 
  //   the components this one does in addition to 'id'
  //  - No includes(id) check is performed by this method,
  //    so adding an already included component is a no-op
  EntityPrototype extend(ComponentProtoId id) const;

  // Returns a new EntityPrototype which includes all
  //   the components this one does EXCEPT 'id'
  //  - Analogously to extend() this method is a no-op
  //    when the prototype doesn't include 'id'
  EntityPrototype drop(ComponentProtoId id) const;

  void dbg_PrintComponents() const;

private:
  static ComponentProtoId find_and_clear_lsb(u64 *components);

  EntityPrototype(ComponentTypeMap components);

  template <typename Fn>
  static void foreach_bit_qword(u64 components, u64 base, Fn&& fn)
  {
    while(components) {
      auto next = find_and_clear_lsb(&components);

      fn(base + next);
    }
  }

  ComponentTypeMap m_components;
};

inline bool operator==(const EntityPrototype& a, const EntityPrototype& b)
{
  return a.equal(b);
}

inline bool operator!=(const EntityPrototype& a, const EntityPrototype& b)
{
  return !a.equal(b);
}

}
