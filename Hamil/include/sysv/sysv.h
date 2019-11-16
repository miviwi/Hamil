#pragma once

#include <common.h>

#include <config>

namespace sysv {

void init();
void finalize();

namespace cpuinfo_detail {
// Forward declaration
class ProcCPUInfo;

ProcCPUInfo *cpuinfo();
}

namespace thread_detail {
extern const int ThreadSuspendResumeSignal;

struct ThreadSuspendResumeAction {
  enum : int {
    Invalid,
    Suspend, Resume,
  };

  u32 initiator;

  int action = Invalid;
  void *thread_data = nullptr;
};
}

// Forward declaration
class X11Connection;

namespace x11_detail {
X11Connection& x11();
}

}
