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
    // Source: https://stackoverflow.com/questions/13358672
    using Traits = util::LambdaTraits<decltype(&Fn::operator())>;
    using T = Traits::ArgType;

    foreach<typename T::RefType>(Traits::to_function(fn));
  }

  template <typename T>
  void foreach(std::function<void(ComponentRef<T> component)> fn)
  {
    components().foreach<T>([&](auto component) {
      fn({ component });
    });
  }

protected:
  virtual ComponentStore& components() = 0;
};

IComponentManager::Ptr create_component_manager();

}