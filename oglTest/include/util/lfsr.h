#pragma once

#include <common.h>

namespace util {

// Implements a Maximum-Length Linear-feeback shift register i.e.
//   one which goes through all possible N-bit values (N == 32 here)
//   before cycling
// Useful for generating unique random identifiers for example
class MaxLength32BitLFSR {
public:
  // source: https://users.ece.cmu.edu/~koopman/lfsr/32.txt
  static constexpr u32 Feedback = 0x8000'0EA69;

  MaxLength32BitLFSR(u32 seed = 1);

  u32 next();

private:
  u32 m;
};

}