#pragma once

#include <common.h>

namespace game {

class IComponentManager;
class IEntityManager;

void init();
void finalize();

IComponentManager& components();
IEntityManager& entities();


void frame();

}