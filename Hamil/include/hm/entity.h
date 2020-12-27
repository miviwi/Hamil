#pragma once

#include <hm/hamil.h>
#include <hm/componentref.h>

#include <utility>

namespace hm {

struct GameObject;

class Entity {
public:
  enum : EntityId {
    Invalid = 0
  };

  Entity(EntityId id = Invalid);

  EntityId id() const;

  // Returns 'true' when id != Invalid
  operator bool() const;

  bool operator==(const Entity& other) const;
  bool operator!=(const Entity& other) const;

  // Returns 'false' when the entity was destroyed
  bool alive() const;

  void destroy();

  template <typename T>
  bool hasComponent()
  {
    return component<T>();
  }

  template <typename T>
  ComponentRef<T> component()
  {
    return {nullptr};
  }

  template <typename T, typename... Args>
  ComponentRef<T> addComponent(Args&&... args)
  {
    return {nullptr};
  }

  template <typename T>
  void removeComponent()
  {
  }

  GameObject& gameObject();

private:
  EntityId m_id;
};

}
