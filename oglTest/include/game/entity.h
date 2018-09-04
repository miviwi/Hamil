#pragma once

#include <game/game.h>
#include <game/componentref.h>

#include <utility>

namespace game {

using EntityId = u32;

class Entity {
public:
  enum : EntityId {
    Invalid = 0
  };

  Entity(EntityId id = Invalid);

  EntityId id() const;

  // Returns 'true' when id != Invalid
  operator bool() const;

  // Returns 'false' when the entity was destroyed
  bool alive() const;

  template <typename T>
  bool hasComponent()
  {
    return component<T>();
  }

  template <typename T>
  ComponentRef<T> component()
  {
    return components().getComponentById<T>(id());
  }

  template <typename T, typename... Args>
  ComponentRef<T> addComponent(Args... args)
  {
    return components().createComponent<T>(id(), std::forward<Args>(args)...);
  }

  template <typename T>
  void removeComponent()
  {
    components().removeComponent<T>(id());
  }

private:
  EntityId m_id;
};

}