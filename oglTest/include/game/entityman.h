#pragma once

#include <game/game.h>
#include <game/entity.h>
#include <game/componentman.h>

#include <memory>
#include <string>

namespace game {

class IComponentManager;

class IEntityManager {
public:
  using Ptr = std::unique_ptr<IEntityManager>;

  virtual Entity createEntity() = 0;
  virtual void destroyEntity(EntityId id) = 0;

  // Creates an Entity with a 'GameObject' Component
  virtual Entity createGameObject(const std::string& name) = 0;

  virtual bool alive(EntityId id) = 0;
};

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man);

}