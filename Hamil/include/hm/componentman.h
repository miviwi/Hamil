#pragma once

#include <hm/hamil.h>
#include <hm/component.h>
#include <hm/componentref.h>

#include <components.h>

#include <util/lambdatraits.h>

#include <functional>
#include <memory>
#include <utility>

namespace hm {

class IComponentManager {
public:
  using Ptr = std::shared_ptr<IComponentManager>;

  template <typename T>
  ComponentRef<T> getComponentById(EntityId entity)
  {
    return components().getComponentById<T>(entity);
  }

  template <typename T, typename... Args>
  ComponentRef<T> createComponent(EntityId entity, Args... args)
  {
    return components().createComponent<T>(entity, std::forward<Args>(args)...);
  }

  template <typename T>
  void removeComponent(EntityId entity)
  {
    return components().removeComponent<T>(entity);
  }

  template <typename Fn>
  void foreach(Fn fn)
  {
    using Traits = util::LambdaTraits<Fn>;
    using T = Traits::ArgType<0>;

    foreach<typename T::RefType>(Traits::to_function(fn));
  }

  template <typename T>
  void foreach(std::function<void(ComponentRef<T> component)> fn)
  {
    components().foreach<T>(fn);
  }

protected:
  virtual ComponentStore& components() = 0;
};

IComponentManager::Ptr create_component_manager();

}