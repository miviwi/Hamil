#pragma once

#include <common.h>

namespace os {

// Forward declaration
class CpuInfo;

void init();
void finalize();

CpuInfo& cpuinfo();

}
