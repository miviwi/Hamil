#pragma once

#include <game/game.h>
#include <game/component.h>
#include <game/componentref.h>

#include <components.h>

#include <functional>
#include <memory>
#include <utility>

namespace game {

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