#pragma once

#include <game/game.h>

namespace game {

using EntityId = u32;

class Entity {
public:
  enum : EntityId {
    Invalid = 0
  };

  Entity(EntityId id = Invalid);

  EntityId id() const;

  template <typename T>
  bool hasComponent()
  {
    return component<T>();
  }

  template <typename T>
  T *component()
  {
    return components().getComponentById<T>(id());
  }

private:
  EntityId m_id;
};

}