#pragma once

#include <common.h>

namespace hm {

class IComponentManager;
class IEntityManager;

void init();
void finalize();

IComponentManager& components();
IEntityManager& entities();

void frame();

}