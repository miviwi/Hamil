#pragma once

#include <game/game.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace game {

template <typename... Args>
struct ComponentStoreBase {
  using HashArray  = std::array<util::HashIndex, sizeof...(Args)>;
  using Components = std::tuple<Args...>;

  HashArray  hashes;
  Components components;
};

}