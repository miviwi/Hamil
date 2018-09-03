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

private:
  Entity m_entity;
};

}