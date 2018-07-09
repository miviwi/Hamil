#pragma once

#include <common.h>

#include <string>
#include <vector>

namespace res {

class ResourceManager;

void init();
void finalize();

ResourceManager& resource();

void resourcegen(std::vector<std::string> resources);

}