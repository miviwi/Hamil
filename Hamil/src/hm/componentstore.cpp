#include <hm/componentstore.h>
#include <hm/component.h>
#include <hm/componentman.h>

#include <components.h>
#include <hm/components/all.h>

namespace hm {

bool IComponentStore::compare_component(u32 id, Component *component)
{
  return id == component->entity().id();
}

void IComponentStore::reap_component(Component *component)
{
  if(!component->entity()) return;

  component->m_entity = Entity();
}

template <typename Tuple>
struct VectorsReserveHelper;

template <typename... Args>
struct VectorsReserveHelper<std::tuple<Args...>> {
  static void reserve(std::tuple<Args...>& vectors, size_t sz)
  {
    (std::get<Args>(vectors).reserve(sz), ...);
  }
};

IComponentStore *create_component_store(size_t hash_size, size_t components_size)
{
  ComponentStore *component_store = new ComponentStore();

  for(auto& hash : component_store->hashes) {
    hash = util::HashIndex(hash_size, hash_size);
  }

  VectorsReserveHelper<ComponentStore::Components>::reserve(
    component_store->components, components_size
  );

  return component_store;
}

}