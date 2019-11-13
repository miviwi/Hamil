#pragma once

#include <win32/win32.h>

namespace os {
// Forward declaration
class CpuInfo;
}

namespace win32 {

os::CpuInfo *create_cpuinfo();

}
