#pragma once

#include <type_traits>
#include <new>
#include <memory>
#include <tuple>
#include <array>
#include <utility>

#include <cstddef>

namespace util {

// Forward declaration
template <typename... Ts>
class AbstractTuple;

// Partial specializations below
template <typename Tuple>
struct AbstractTupleBuilder;

class IAbstractTuple {
public:
  // Returns a pointer to a heap allocated AbstractTuple as IAbstractTuple*
  //   - XXX: usage of this method should be avoided whenever possible
  //        because it's very slow (do we even need it?)
  template <typename... Args>
  static std::unique_ptr<IAbstractTuple> from_args(Args&&... args)
  {
    return new AbstractTuple<Args...>(std::forward<Args&&>(args)...);
  }

  // Creates an AbstractTuple in a pre-allocated buffer
  //   - 'storage' MUST point to a buffer sized >= AbstractTupleBuilder<...>::Size!
  //   - Returns a pointer to the storage cast to IAbstractTuple*
  //   - Caller is responsible for cleanup
  template <typename... Args>
  static IAbstractTuple *from_args_with_storage(void *storage, Args&&... args)
  {
    using Tuple = AbstractTuple<Args...>;

    auto t = (Tuple *)storage;    // Must be UNINITIALIZED!

    // Initialize storage with the provided args
    new(t) Tuple(std::forward<Args&&>(args)...);

    return t;
  }

  virtual ~IAbstractTuple() = default;

  template <typename... Ts>
  std::tuple<Ts...>& get()
  {
    return *(std::tuple<Ts...> *)rawPtr();
  }

  template <typename... Ts>
  const std::tuple<Ts...>& get() const
  {
    return *(const std::tuple<Ts...> *)rawPtr();
  }

protected:
  virtual void *rawPtr() = 0;
  virtual const void *rawPtr() const = 0;
};

template <typename... Ts>
struct AbstractTupleBuilder<std::tuple<Ts...>> {

  static constexpr size_t Size = sizeof(AbstractTuple<Ts...>);

  template <typename T = std::byte>
  static constexpr size_t SizeForStorage = (Size + (sizeof(T)-1)) / sizeof(T);
  //                                        ^^^^^^^^^^^^^^^^^^^^
  //                              Round the quotient UP => ceil(Size/sizeof(T))

  template <typename T = std::byte>
  using Storage = std::array<T, SizeForStorage<T>>;

  static IAbstractTuple *build(Ts&&... args)
  {
    return IAbstractTuple::from_args(std::forward<Ts&&>(args)...);
  }

  template <typename T>
  static IAbstractTuple *build(Storage<T>& storage, Ts&&... args)
  {
    return IAbstractTuple::from_args_with_storage(
        storage.data(), std::forward<Ts&&>(args)...
    );
  }

  template <typename BufItem, size_t N>
  static IAbstractTuple *build(BufItem (&storage)[N], Ts&&... args)
  {
    static_assert(sizeof(BufItem)*N >= Size,
        "the specified buffer is too small to store the AbstractTuple!");

    return IAbstractTuple::from_args_with_storage(
        storage, std::forward<Ts&&>(args)...
    );
  }

private:
  AbstractTupleBuilder() = default;    // Prevent instantiation of these, as
                                       //   the AbstractTupleBuilder is a type
};                                     //   only to allow partial specialization

template <typename... Ts>
class AbstractTuple final : public IAbstractTuple {
public:
  using Tuple = std::tuple<Ts...>;

  template <typename... Args>
  AbstractTuple(Args... args) :
    m_self(std::forward<Args>(args)...)
  {
  }

protected:
  virtual void *rawPtr() final { return &m_self; }
  virtual const void *rawPtr() const final { return &m_self; }

private:
  Tuple m_self;
};

}
