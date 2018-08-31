#pragma once

#include <game/game.h>

namespace game {

class Entity {
public:
  using Id = u32;

  enum : Id {
    Invalid = 0
  };

  Entity(Id id = Invalid);

private:
  Id m_id;
};

}