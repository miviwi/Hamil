#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/componentman.h>

#include <memory>
#include <string>

namespace hm {

class IComponentManager;

class IEntityManager {
public:
  using Ptr = std::unique_ptr<IEntityManager>;

  virtual Entity createEntity() = 0;
  virtual Entity findEntity(const std::string& name) = 0;
  virtual void destroyEntity(EntityId id) = 0;

  // Creates an Entity with a 'GameObject' Component
  virtual Entity createGameObject(const std::string& name, Entity parent) = 0;

  Entity createGameObject(const std::string& name)
  {
    return createGameObject(name, Entity::Invalid);
  }

  virtual bool alive(EntityId id) = 0;
};

IEntityManager::Ptr create_entity_manager(IComponentManager::Ptr component_man);

}