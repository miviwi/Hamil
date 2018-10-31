#pragma once

#include <common.h>

namespace win32 {

class CpuInfo;

void init();
void finalize();

CpuInfo& cpuinfo();

}