#pragma once

#include <os/time.h>

namespace win32 {

using Time = os::Time;

class Timers {
public:
  static void init();
  static void finalize();

  static void tick();
  static Time ticks();

  static Time ticks_per_s();
};


}
