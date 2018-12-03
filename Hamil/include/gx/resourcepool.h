#pragma once

#include <gx/gx.h>
#include <gx/program.h>
#include <gx/buffer.h>
#include <gx/framebuffer.h>
#include <gx/texture.h>
#include <gx/vertex.h>
#include <gx/renderpass.h>
#include <gx/fence.h>

#include <util/smallvector.h>
#include <util/tupleindex.h>
#include <win32/mutex.h>

#include <vector>
#include <array>
#include <tuple>
#include <utility>

namespace gx {

// The pool_size is not enforced, so due to std::vector
//   iterator/reference invalidaton semantics reource pointers
//   can become invalid when it is exceeded
class ResourcePool {
public:
  using Id = ::u32;

  enum : Id {
    Invalid = ~0u,
  };

  template <typename... Args>
  using TupleOfVectors = std::tuple<std::vector<Args>...>;

  using FreeList = util::SmallVector<Id, 64>;

  // 'pool_size' represents the number of pre-allocated
  //   Ids per resource type
  ResourcePool(uint32_t pool_size);
  ResourcePool(const ResourcePool& other) = delete;

  template <typename T, typename... Args>
  Id create(Args... args)
  {
    auto lock_guard = m_mutex.acquireScoped();

    FreeList& free_list = getFreeList<T>();
    auto& vec = getVector<T>();

    T *resource = nullptr;
    Id id = Invalid;

    if(free_list.empty()) {
      id = (Id)vec.size();

      vec.emplace_back(std::forward<Args>(args)...);
    } else {
      id = free_list.pop();
      resource = vec.data() + id;

      resource->~T();
      new(resource) T(std::forward<Args>(args)...);
    }

    return id;
  }

  // See gx.h for notes on 'label' strings
  template <typename T, typename... Args>
  Id create(const char *label, Args... args)
  {
    auto lock_guard = m_mutex.acquireScoped();

    FreeList& free_list = getFreeList<T>();
    auto& vec = getVector<T>();

    T *resource = nullptr;
    Id id = Invalid;

    if(free_list.empty()) {
      id = (Id)vec.size();

      resource = &vec.emplace_back(std::forward<Args>(args)...);
    } else {
      id = free_list.pop();
      resource = vec.data() + id;

      resource->~T();
      new(resource) T(std::forward<Args>(args)...);
    }

    resource->label(label);

    return id;
  }

  template <typename T>
  void release(Id id)
  {
    FreeList& free_list = getFreeList<T>();
    auto& vec = getVector<T>();

    assert(id < vec.size() && "Invalid 'id' passed to release()!");
    assert(vec.at(id).refs() == 1 && "Attempted to release() an in-use resource!");

    // Calling the destructor is deferred until the
    //   resource is re-used
    free_list.append(id);
  }

  void releaseTexture(Id id)
  {
    release<TextureHandle>(id);
  }

  void releaseBuffer(Id id)
  {
    release<BufferHandle>(id);
  }

  template <typename T, typename... Args>
  Id createTexture(Args... args)
  {
    return create<TextureHandle>(TextureHandle::create<T>(std::forward<Args>(args)...));
  }

  // See gx.h for notes on 'label' strings
  template <typename T, typename... Args>
  Id createTexture(const char *label, Args... args)
  {
    return create<TextureHandle>(label, TextureHandle::create<T>(std::forward<Args>(args)...));
  }

  template <typename T, typename... Args>
  Id createBuffer(Args... args)
  {
    return create<BufferHandle>(BufferHandle::create<T>(std::forward<Args>(args)...));
  }

  // See gx.h for notes on 'label' strings
  template <typename T, typename... Args>
  Id createBuffer(const char *label, Args... args)
  {
    return create<BufferHandle>(label, BufferHandle::create<T>(std::forward<Args>(args)...));
  }

  template <typename T> T& get(Id id) { return getVector<T>().at(id); }
  template <typename T> const T& get(Id id) const { return getVector<T>().at(id); }

  template <typename T> T& getTexture(Id id) { return getTexture(id).get<T>(); }
  TextureHandle& getTexture(Id id) { return get<TextureHandle>(id); }

  template <typename T> const T& getTexture(Id id) const { return getTexture(id).get<T>(); }
  const TextureHandle& getTexture(Id id) const { return get<TextureHandle>(id); }

  template <typename T> T& getBuffer(Id id) { return getBuffer(id).get<T>(); }
  BufferHandle& getBuffer(Id id) { return get<BufferHandle>(id); }

  template <typename T> const T& getBuffer(Id id) const { return getBuffer(id).get<T>(); }
  const BufferHandle& getBuffer(Id id) const { return get<BufferHandle>(id); }

  template <typename T>
  T& operator()(Id id)
  {
    return get<T>(id);
  }

  // Destroys all resources which are referenced
  //   by ONLY this ResourcePool
  void purge();

private:
  template <typename T>
  std::vector<T>& getVector()
  {
    return std::get<std::vector<T>>(m_resources);
  }

  template <typename T>
  const std::vector<T>& getVector() const
  {
    return std::get<std::vector<T>>(m_resources);
  }

  template <typename T>
  FreeList& getFreeList()
  {
    auto idx = util::tuple_index_v<std::vector<T>, decltype(m_resources)>;

    return m_free_lists.at(idx);
  }

  template <typename T>
  const FreeList& getFreeList() const
  {
    auto idx = util::tuple_index_v<std::vector<T>, decltype(m_resources)>;

    return m_free_lists.at(idx);
  }

  win32::Mutex m_mutex;

  TupleOfVectors<
    Program,
    VertexArray,
    IndexedVertexArray,
    Framebuffer,
    TextureHandle,
    Sampler,
    BufferHandle,
    RenderPass,
    Fence
  > m_resources;

  enum {
    NumResources = std::tuple_size_v<decltype(m_resources)>,
  };

  std::array<FreeList, NumResources> m_free_lists;
};

}