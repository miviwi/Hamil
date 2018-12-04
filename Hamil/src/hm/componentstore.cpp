#include <hm/componentstore.h>
#include <hm/component.h>
#include <hm/componentman.h>

#include <components.h>
#include <hm/components/all.h>

#include <cassert>

namespace hm {

bool IComponentStore::compare_component(u32 id, Component *component)
{
  return id == component->entity().id();
}

void IComponentStore::lock()
{
  int locked = m_locked.load();

  // Take the mutex if we're the first lock() invocation
  //   or if requireUnlocked() was called
  if(locked <= 0) {
    // m_mutex is held until endRequireUnlocked() was
    //   called as many times as requireUnlocked()
    //   (so locked == 0)
    m_mutex.acquire();
  }

  // If we were woken up after another thread waiting
  //   on 'm_mutex' the counter could is now in an
  //   unexpected state
  if(!m_locked.compare_exchange_strong(locked, locked+1)) {
    // We'll need to reacquire the mutex because we
    //   slept through a requireUnlocked() call
    m_mutex.release();

    lock();   // Retry
  }
}

void IComponentStore::unlock()
{
  int locked = m_locked.fetch_sub(1);
  assert(locked > 0 && "unlock() called more times than lock()!");

  // The last unlock() releases the mutex
  if(locked == 1) m_mutex.release();
}

void IComponentStore::requireUnlocked()
{
  int locked = m_locked.load();

  if(locked >= 0) {
    // m_mutex is held until unlock() was
    //   called as many times as lock()
    //   (so locked == 0)
    m_mutex.acquire();
  }

  if(!m_locked.compare_exchange_strong(locked, locked-1)) {
    // We'll need to reacquire the mutex because we
    //   slept through a lock() call
    m_mutex.release();

    requireUnlocked();  // Retry
  }
}

void IComponentStore::endRequireUnlocked()
{
  int locked = m_locked.fetch_add(1);
  assert(locked < 0 && "endRequireUnlocked() called more times than requireUnlocked()!");

  // The last endRequireUnlocked() releases the mutex
  if(locked == -1) m_mutex.release();
}

void IComponentStore::reap_component(Component *component)
{
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