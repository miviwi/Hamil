#pragma once

#include <common.h>

namespace res {

class ResourceManager;

void init();
void finalize();

ResourceManager& resource();

}