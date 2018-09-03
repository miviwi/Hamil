#pragma once

#include <game/game.h>
#include <game/entity.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace game {

class Component {
public:
  EntityId entity() const;

private:
  EntityId m_entity;
};

}