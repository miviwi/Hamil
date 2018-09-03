#pragma once

#include <game/game.h>

#include <util/hashindex.h>
#include <util/tupleindex.h>

#include <array>
#include <vector>
#include <tuple>
#include <type_traits>

namespace game {

class Component;

struct IComponentStore {
public:
  static bool compare_component(u32 id, Component *component);
};

template <typename... Args>
struct ComponentStoreBase : IComponentStore {
  using HashArray  = std::array<util::HashIndex, sizeof...(Args)>;
  using Components = std::tuple<std::vector<Args>...>;

  HashArray  hashes;
  Components components;

  template <typename T>
  T *getComponentById(u32 id)
  {
    static_assert(std::is_base_of_v<Component, T>, "T must be a Component!");

    constexpr auto TupleIndex = util::tuple_index_v<std::vector<T>, Components>;

    util::HashIndex& hash  = hashes[TupleIndex];
    std::vector<T>& bucket = std::get<TupleIndex>(components);

    auto index = hash.find(id, [&](u32 id, u32 index) {
      const auto elem = bucket.data() + index;
      return compare_component(id, elem);
    });

    return index != util::HashIndex::Invalid ? (bucket.data() + index) : nullptr;
  }
};

IComponentStore *create_component_store(size_t hash_size, size_t components_size);

template <typename T>
static T *create_component_store(size_t hash_size, size_t components_size)
{
  return (T *)create_component_store(hash_size, components_size);
}

}