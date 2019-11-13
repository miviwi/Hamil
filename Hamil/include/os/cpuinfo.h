#pragma once

#include <os/os.h>

namespace win32 {
// Forward declaration
os::CpuInfo *create_cpuinfo();
}

namespace sysv {
// Forward declaration
os::CpuInfo *create_cpuinfo();
}

namespace os {

class CpuInfo {
public:
  // Returns the number of physical CPU cores available
  uint numPhysicalProcessors() const;

  // Returns the number of CPU threads available
  //   - Can be >= numPhysicalProcessors() depending
  //     on the system.
  uint numLogicalProcessors() const;

  // Returns 'true' when each core is capable of running
  //   more than 1 thread simultaneously
  bool hyperthreading() const;

private:
  // void os::init();
  friend void init();

  friend CpuInfo *win32::create_cpuinfo();
  friend CpuInfo *sysv::create_cpuinfo();

  CpuInfo() = default;

  static CpuInfo *create();

  uint m_num_physical_processors;
  uint m_num_logical_processors;
};

}
