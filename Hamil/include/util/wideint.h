#pragma once

#include <cstdint>
#include <climits>

union u128 {
  struct {
    uint64_t lo, hi;      // Little-endian LSB first
  };

  struct {
    uint8_t bytes[sizeof(uint64_t)*2];
  };
};

union u256 {
  struct {
    u128 lo, hi;      // Little-endian LSB first
  };

  struct {
    uint64_t quadwords[(sizeof(u128)*2) / sizeof(uint64_t)];
  };

  struct {
    uint8_t bytes[sizeof(u128)*2];
  };
};

