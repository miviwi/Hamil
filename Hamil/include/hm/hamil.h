#pragma once

#include <common.h>

namespace hm {

class IComponentManager;
class IEntityManager;

using EntityId = u32;

void init();
void finalize();

IComponentManager& components();
IEntityManager& entities();

void frame();

}
