#pragma once

#include <win32/win32.h>

namespace win32 {

class CpuInfo {
public:

  // Returns the number of physical CPU cores available
  uint numPhysicalProcessors() const;

  // Returns the number of CPU threads available
  //   - Can be >= numPhysicalProcessors() depending
  //     on the system.
  uint numLogicalProcessors() const;

  // Returns 'true' when each core is capable of running
  //   more than 1 thrad simultaneously
  bool hyperthreading() const;

protected:
  CpuInfo() = default;

private:
  friend void init();

  static CpuInfo *create();

  uint m_num_physical_processors;
  uint m_num_logical_processors;
};

}