#pragma once

#include <common.h>

#include <string>

namespace win32 {

class StdStream {
public:
  static void init();

  static std::string gets();
};


}