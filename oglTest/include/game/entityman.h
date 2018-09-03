#pragma once

#include <game/game.h>
#include <game/entity.h>
#include <game/componentman.h>

#include <memory>

namespace game {

class IComponentManager;

class IEntityManager {
public:
  using Ptr = std::unique_ptr<IEntityManager>;

  virtual Entity createEntity() = 0;
};

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man);

}