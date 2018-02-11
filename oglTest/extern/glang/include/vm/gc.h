#pragma once

#include <cstdint>

namespace glang {

union GCHeader {
  struct {
    uint32_t color : 2;

    // Number of heap blocks until next used one
    uint32_t next : 30;
  };

  uint32_t raw;
};

class GC {
public:
  enum : uint32_t {
    HeapEndMarker = ~0u,

    ColorFree  = 0,
    ColorWhite = 1,
    ColorGrey  = 2,
    ColorBlack = 3,
  };

};

}