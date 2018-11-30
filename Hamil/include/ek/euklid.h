#pragma once

#include <common.h>

namespace ek {

class Renderer;

void init();
void finalize();

Renderer& renderer();

}