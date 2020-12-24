#include <hm/prototype.h>
#include <hm/prototypechunk.h>

#include <components.h>
#include <hm/components/all.h>

#include <util/hash.h>

#include <config>

#include <cassert>
#include <cstdio>
#include <cstddef>

namespace hm {

// Find and clear the first (lsb) set bit and return it's index
/*INTRIN_INLINE*/ ComponentProtoId EntityPrototype::find_and_clear_lsb(u64 *components)
{
  ComponentProtoId id;
#if __win32
  _BitScanForward64(&id, *components);
#else
  id = __builtin_ffsll(*components)-1;
#endif
  *components &= *components - 1;

  return id;
}

EntityPrototype::Hash EntityPrototype::hash_type_map(const ComponentTypeMap& components)
{
  const auto h = util::ByteXXHash(sizeof(components.bits));

  const auto wide_hash = h(components.bits.bytes);

  // ByteXXHash yields -> a 64-bit wide hash, but because we'll be
  //   using these hashes as util::HashIndex keys converting them
  //   to 32-bit wide is necessary...
  const u32 hlo = (u32)(wide_hash & 0xFFFFFFFFull),
        hhi = (u32)(wide_hash >> 32ull);

  //  ...use a simple XOR for this
  return hlo ^ hhi;
}

EntityPrototype::EntityPrototype(
    std::initializer_list<ComponentProtoId> components
  ) : m_components(ComponentTypeMap::zero())
{
  for(auto c : components) {
    m_components.setMut(c);
  }
}

EntityPrototype::EntityPrototype(ComponentTypeMap components) :
  m_components(components)
{
}

bool EntityPrototype::equal(const ComponentTypeMap& other) const
{
  return m_components == other;
}

bool EntityPrototype::equal(const EntityPrototype& other) const
{
  return equal(other.m_components);
}

bool EntityPrototype::includes(const EntityPrototype& other) const
{
  return m_components.bitAnd(other.m_components) == other.m_components;
}

bool EntityPrototype::includes(ComponentProtoId component) const
{
  return m_components.test(component);
}

const EntityPrototype::ComponentTypeMap& EntityPrototype::components() const
{
  return m_components;
}

size_t EntityPrototype::numProtoComponents() const
{
  return m_components.popcount();
}

size_t EntityPrototype::chunkCapacity() const
{
  const auto entity_size = componentDataSizePerEntity();
  const auto chunk_size  = PrototypeChunkSize;

  return chunk_size / entity_size;
}

size_t EntityPrototype::componentDataSizePerEntity() const
{
  size_t sz = 0;
  foreachProtoId([&](ComponentProtoId id) {
    const auto metaclass = metaclass_from_protoid(id);

    // Accumulate sizes of all Components included in this prototype...
    sz += metaclass->staticData().data_size;
  });

  return sz;
}

size_t EntityPrototype::componentDataOffsetInAoSEntity(
    ComponentProtoId component
  ) const
{
  // XXX: move this to a run-time check maybe?
  //   - alternatively offer a 'safe' (with the check)
  //     and 'unsafe' (no runtime check) version of
  //     this method
  assert(includes(component) && "EntityPrototype doesn't include 'component'!");

  // The offset is calculated as the size of an EntityPrototype where
  //    all components of this prototype whose bit index < 'component'
  //    are ANDed wth 1-bits (i.e. left unchanged) and all other bits
  //    (ones >= 'component' bit index) cleared
  auto mask = ComponentTypeMap::mask_n_lsbs(component);
  auto masked_proto = EntityPrototype(m_components.bitAnd(mask));

  return masked_proto.componentDataSizePerEntity();
}

size_t EntityPrototype::componentDataOffsetInSoAEntityChunk(
    ComponentProtoId component
  ) const
{
  const size_t num_entities_per_chunk = chunkCapacity(),
        offset_in_aos_layout = componentDataOffsetInAoSEntity(component);

  return offset_in_aos_layout * num_entities_per_chunk;
}

EntityPrototype::Hash EntityPrototype::hash() const
{
  return hash_type_map(m_components);
}

EntityPrototype EntityPrototype::extend(ComponentProtoId id) const
{
  auto extended_components = m_components
    .set(id);

  return EntityPrototype(extended_components);
}

EntityPrototype EntityPrototype::drop(ComponentProtoId id) const
{
  auto dropped_components = m_components
    .clear(id);

  return EntityPrototype(dropped_components);
}

void EntityPrototype::dbg_PrintComponents() const
{
#if !defined(NDEBUG)
  foreachProtoId([this](hm::ComponentProtoId id) {
    auto mc = hm::metaclass_from_protoid(id);

    printf(
        "%12s(%.3u): size=%zu\n"
        "            offset:aos=0x%.4zx offset:chunk=0x%.4zx\n",
        protoid_to_str(id).data(), id, mc->staticData().data_size,
        componentDataOffsetInAoSEntity(id),
        componentDataOffsetInSoAEntityChunk(id)
    );
  });
#endif
}

}
