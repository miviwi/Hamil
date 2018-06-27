#pragma once

#include <common.h>

#include <win32/handle.h>

namespace win32 {

// Copied from msdn
enum WaitResult : ulong {
  WaitObject0   = 0x00000000,
  WaitAbandoned = 0x00000080,
  WaitTimeout   = 0x00000102,
  WaitFailed    = 0xFFFFFFFF,
};

enum : ulong {
  WaitInfinite  = 0xFFFFFFFF,
};

class Waitable : public Handle {
public:
  virtual WaitResult wait(ulong timeout = WaitInfinite);
};

}