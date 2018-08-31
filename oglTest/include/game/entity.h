#pragma once

#include <game/game.h>

#include <util/lfsr.h>

namespace game {

using EntityId = u32;

class EntityManager {
public:

  EntityId newId();

private:
  util::MaxLength32BitLFSR m_next_id;
};

class Entity {
public:
  enum : EntityId {
    Invalid = 0
  };

  Entity(EntityId id = Invalid);

private:
  EntityId m_id;
};

}