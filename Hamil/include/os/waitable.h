#pragma once

#include <common.h>
#include <util/ref.h>

namespace os {

// Copied from msdn
enum WaitResult : ulong {
  WaitObject0   = 0x00000000,

  WaitFailed    = 0x8000'0000,
  WaitAbandoned = 0x8000'0001,
  WaitTimeout   = 0x8000'0002
};

enum : ulong {
  WaitInfinite  = 0xFFFFFFFF,
};

class Waitable : public Ref {
public:
  virtual ~Waitable();

  virtual WaitResult wait(ulong timeout_ms = WaitInfinite) = 0;
};

}
