#pragma once

#include <game/game.h>
#include <game/component.h>

#include <components.h>

#include <memory>

namespace game {

class IComponentManager {
public:
  template <typename T>
  T *getComponentById(EntityId entity)
  {
    return components().getComponentById<T>(entity);
  }

protected:
  virtual ComponentStore& components() = 0;
};

std::unique_ptr<IComponentManager> create_component_manager();

}