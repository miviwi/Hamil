#include <gx/resourcepool.h>

#include <cassert>

namespace gx {

template <typename Tuple>
struct TupleOfVectorsHelper;

template <typename... Args>
struct TupleOfVectorsHelper<std::tuple<Args...>> {
  using Tuple = std::tuple<Args...>;

  static void reserve(Tuple& vectors, size_t capacity)
  {
    (std::get<Args>(vectors).reserve(capacity), ...);
  }

  static void clear(Tuple& vectors)
  {
    (std::get<Args>(vectors).clear(), ...);
  }
};

ResourcePool::ResourcePool(uint32_t pool_size)
{
  TupleOfVectorsHelper<decltype(m_resources)>::reserve(m_resources, pool_size);
}

void ResourcePool::purge()
{
  TupleOfVectorsHelper<decltype(m_resources)>::clear(m_resources);

  size_t sz = std::get<0>(m_resources).size();
  for(auto& free_list : m_free_lists) {
    free_list.clear();
  }
}

}