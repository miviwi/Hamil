#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/component.h>

#include <util/hashindex.h>
#include <util/tupleindex.h>

#include <array>
#include <vector>
#include <tuple>
#include <utility>
#include <type_traits>

namespace hm {

class Component;

class IComponentStore {
public:
  static bool compare_component(u32 id, Component *component);

protected:
  static void reap_component(Component *component);
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
    checkComponent<T>();

    constexpr auto TupleIndex = tuple_index<T>();

    util::HashIndex& hash  = getHash<TupleIndex>();
    std::vector<T>& bucket = getBucket<TupleIndex>();

    auto index = findComponent<TupleIndex>(id);

    return index != util::HashIndex::Invalid ? (bucket.data() + index) : nullptr;
  }

  template <typename T, typename... Args>
  T *createComponent(u32 id, Args... args)
  {
    checkComponent<T>();

    constexpr auto TupleIndex = tuple_index<T>();

    util::HashIndex& hash  = getHash<TupleIndex>();
    std::vector<T>& bucket = getBucket<TupleIndex>();

    auto index = (util::HashIndex::Index)bucket.size();

    bucket.emplace_back(id, std::forward<Args>(args)...);
    auto component = &bucket.back();

    hash.add(id, index);

    return component;
  }

  template <typename T>
  void removeComponent(u32 id)
  {
    checkComponent<T>();

    constexpr auto TupleIndex = tuple_index<T>();

    util::HashIndex& hash  = getHash<TupleIndex>();
    std::vector<T>& bucket = getBucket<TupleIndex>();

    auto index = findComponent<TupleIndex>(id);
    auto component = bucket.data() + index;

    component->destroyed();

    reap_component(component);
    hash.remove(id, index);
  }

  template <typename T, typename Fn>
  void foreach(Fn fn)
  {
    checkComponent<T>();

    constexpr auto TupleIndex = tuple_index<T>();

    std::vector<T>& bucket = getBucket<TupleIndex>();
    for(auto& el : bucket) {
      auto& component = (Component&)el;
      if(!component) {
        reap_component(&el);
        continue;
      }

      fn(&el);
    }
  }

private:
  template <typename T>
  void checkComponent()
  {
    static_assert(std::is_base_of_v<Component, T>, "T must be a Component!");
  }

  template <typename T>
  static constexpr size_t tuple_index()
  {
    return util::tuple_index_v<std::vector<T>, Components>;
  }

  template <size_t Idx>
  util::HashIndex& getHash()
  {
    return hashes[Idx];
  }

  template <size_t Idx>
  std::tuple_element_t<Idx, Components>& getBucket()
  {
    return std::get<Idx>(components);
  }

  template <size_t Idx>
  util::HashIndex::Index findComponent(u32 id)
  {
    auto& bucket = getBucket<Idx>();

    return getHash<Idx>().find(id, [&](u32 id, u32 index) {
      const auto elem = bucket.data() + index;
      return compare_component(id, elem);
    });
  }

};

IComponentStore *create_component_store(size_t hash_size, size_t components_size);

template <typename T>
static T *create_component_store(size_t hash_size, size_t components_size)
{
  return (T *)create_component_store(hash_size, components_size);
}

}