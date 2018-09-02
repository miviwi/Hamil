#pragma once

#include <components.h>
#include <game/game.h>
#include <game/entity.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace game {

class Component {

};

class IComponentManager {
public:
  template <typename T>
  T *getComponentById(EntityId entity)
  {
    return nullptr;
  }

protected:
};

}