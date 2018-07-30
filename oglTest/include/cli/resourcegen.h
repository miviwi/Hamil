#pragma once

#include <common.h>

#include <string>
#include <vector>

namespace cli {

struct GenError {
  const std::string what;
  GenError(const std::string& what_) :
    what(what_)
  { }
};

void resourcegen(std::vector<std::string> resources);

}
