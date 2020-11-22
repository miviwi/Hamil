#pragma once

#include <common.h>
#include <util/ref.h>

#include <new>
#include <memory>
#include <type_traits>
#include <utility>
#include <algorithm>

#include <cassert>

namespace polystorage_detail {

struct Allocator {
  static void *alloc(size_t sz);
  static void free(void *ptr, size_t sz);
};

}

// Class used to efficiently implement polymorphic types
//   with PIMPL data members. The efficiency is achieved
//   by inlining the "PIMPL" data into the class so that
//   only one allocation is necessary.
//
// Usage:
//   // The PIMPL data
//   struct MyClassStorage;
//
//   class MyClassBase {
//   public:
//     virtual void doSomething() = 0;
//   };
//
//   class MyClass : public WithPolymorphicStorage<MyClassBase> {
//   public:
//     // 'MyClass' derives indirectly from 'MyClassBase'
//
//     static MyClass *create();
//     static void destroy(MyClass *obj);
//
//     virtual void doSomething() { puts("quack!"); }
//
//   private:
//     MyClass() = default;     // Instances of 'MyClass' MUST be created via
//                              //   a call to WithPolymorphicStorage::alloc(),
//                              //   making the constructor 'private' ensures
//                              //   that's the case
//   };
//
// ------------- In some *.cpp file... -------------
//   struct MyClassStorage {
//     int a, b, c;
//   };
//
//   MyClass *MyClass::create()
//   {
//      return WithPolymorphicStorage<MyClass, MyClassStorage>();
//   }
//
//   void MyClass::destroy(MyClass *obj)
//   {
//     WithPolymorphicStorage::destroy(MyClassStorage>(obj);
//   }
//
template <typename Base, typename Allocator = polystorage_detail::Allocator>
class WithPolymorphicStorage : public Base {
public:
  static constexpr bool UsesDefaultAllocator = std::is_same_v<
      Allocator, polystorage_detail::Allocator
  >;

  using Base::Base;

  virtual ~WithPolymorphicStorage() = default;

  WithPolymorphicStorage(const WithPolymorphicStorage&) = delete;
  
  template <typename Storage, typename Derived>
  static void polystorage_deleter(Derived *ptr)
  {
    static_assert(std::is_base_of_v<WithPolymorphicStorage<Base, Allocator>, Derived>,
        "attempted to get a 'WithPolymorphicStorage' deleter for non-WithPolymorphicStorage type!");

    destroy<Storage>(ptr);
  }

protected:
  // The constructor is made 'protected' so it's impossible to
  //   instantiate a 'WithPolymorphicStorage' object directly
  WithPolymorphicStorage() = default;

  template <typename Derived, typename Storage, typename... Args>
  static Derived *alloc(Args... args)
  {
    auto obj = (Derived *)Allocator::alloc(object_size<Derived, Storage>());
    new(obj) Derived(std::forward<Args>(args)...);

#if !defined(NDEBUG)
    auto obj_polystorage = (WithPolymorphicStorage *)obj;
    obj_polystorage->m_dbg_object_sz = object_size<Derived, Storage>();
#endif

    return obj;
  }

  template <typename Storage, typename Derived>
  static void destroy(Derived *obj)
  {
    auto obj_polystorage = (WithPolymorphicStorage *)obj;
    auto storage = obj_polystorage->template storage<Storage>();

    assert((obj_polystorage->m_dbg_object_sz == object_size<Derived, Storage>()) &&
        "The 'Storage' type passed to WithPolymorphicStorage::destroy() is different "
        "from the one which was used for WithPolymorphicStorage::alloc()!");

    if constexpr(std::is_base_of_v<Ref, Base>) {
      auto ref = (Ref *)obj;

      if(ref->refs() > 1) {      // References to object are still around,
        ref->deref();            //   so don't call the destructor yet
        return;
      }
    }

    storage->~Storage();
    obj->~Derived();

    Allocator::free(obj, object_size<Derived, Storage>());
  }

  template <typename Storage>
  Storage *storage()
  {
    return (Storage *)m_storage;
  }

  template <typename Storage>
  const Storage *storage() const
  {
    return (const Storage *)m_storage;
  }

private:
  template <typename Derived>
  friend class WithPolymorphicStoragePtr;

  template <typename Derived, typename Storage>
  static constexpr size_t object_size()
  {
    static_assert(std::is_base_of_v<WithPolymorphicStorage<Base, Allocator>, Derived>,
        "invalid type used for 'Derived'! (does it inherit from WithPolymorphicStorage<Base>?)");

    return sizeof(Derived) + sizeof(Storage);
  }

#if !defined(NDEBUG)
  size_t m_dbg_object_sz;
#endif

  u8 m_storage[1];
};

template <typename T>
class WithPolymorphicStoragePtr {
public:
  WithPolymorphicStoragePtr() : m_ptr(nullptr) { }
  WithPolymorphicStoragePtr(T *ptr) : m_ptr(ptr) { }

  WithPolymorphicStoragePtr(const WithPolymorphicStoragePtr&) = delete;

  WithPolymorphicStoragePtr(WithPolymorphicStoragePtr&& other) :
    WithPolymorphicStoragePtr()
  {
    std::swap(m_ptr, other.m_ptr);
  }

  WithPolymorphicStoragePtr& operator=(WithPolymorphicStoragePtr&& other)
  {
    std::swap(m_ptr, other.m_ptr);

    return *this;
  }

  WithPolymorphicStoragePtr& operator=(const WithPolymorphicStoragePtr&) = delete;

  ~WithPolymorphicStoragePtr()
  {
    if(!m_ptr) return;

    T::destroy(m_ptr);
  }

  operator bool() { return m_ptr; }

  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T& operator()() { return *m_ptr; }
  const T& operator()() const { return *m_ptr; }

private:
  T *m_ptr;

};
