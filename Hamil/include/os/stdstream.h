#pragma once

#include <os/os.h>

#include <string>

namespace os {

class StdStream {
public:
  static void init();
  static void finalize();

  static std::string gets();
};

}
