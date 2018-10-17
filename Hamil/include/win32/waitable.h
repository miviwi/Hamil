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

// Base class for Win32 objects which can be waited on
//   - Remember to NEVER decrement the ref-count in derived
//     classes destructors, the Handle class handles that
//     for you. If the object stores additional resources
//     which need clean-up use
//         if(refs() > 1) return;
//     at the top of the destructor (instead of calling
//     deref()) to ensure the ref-count will not be 
//     decremented twice
class Waitable : public Handle {
public:
  // The result will be WaitObject0+i for the i-th waited on object
  virtual WaitResult wait(ulong timeout = WaitInfinite);
};

}