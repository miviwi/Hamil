#pragma once

#include <common.h>

namespace cli {

// DO NOT init() any subsystem before calling this function!
//
// - A return value > 0 signifies the application is done
//   executing and it should be passed to exit()
int args(int argc, char *argv[]);

}