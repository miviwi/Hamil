#pragma once

#include <game/game.h>
#include <game/entity.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace game {

class Component {
public:
  Component(Entity e);
  
  Entity entity() const;

  // Only returns 'true' when it's Entity is != Invalid and alive()
  operator bool() const;

private:
  friend class IComponentStore;

  Entity m_entity;
};

}