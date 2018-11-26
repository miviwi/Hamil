#pragma once

#include <common.h>

#include <array>
#include <initializer_list>

namespace res {

class ResourceManager;

void init();
void finalize();

ResourceManager& resources();

ResourceManager& load(std::initializer_list<size_t> ids);
ResourceManager& load(const size_t *ids, size_t sz);

template <size_t N>
ResourceManager& load(const std::array<size_t, N> ids)
{
  return load(ids.data(), ids.size());
}


}