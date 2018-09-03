#pragma once

#include <game/game.h>
#include <game/component.h>

#include <components.h>

#include <memory>
#include <utility>

namespace game {

class IComponentManager {
public:
  using Ptr = std::shared_ptr<IComponentManager>;

  template <typename T>
  T *getComponentById(EntityId entity)
  {
    return components().getComponentById<T>(entity);
  }

  template <typename T, typename... Args>
  T *createComponent(EntityId entity, Args... args)
  {
    return components().createComponent<T>(entity, std::forward<Args>(args)...);
  }

protected:
  virtual ComponentStore& components() = 0;
};

IComponentManager::Ptr create_component_manager();

}