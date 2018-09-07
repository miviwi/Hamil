#pragma once

#include <common.h>

namespace p {
static constexpr size_t HighShift = 32;
static constexpr size_t LowMask = 0xFFFFFFFF;
}

static constexpr u32 dword_high(size_t x)
{
  return (u32)(x >> p::HighShift);
}

static constexpr u32 dword_low(size_t x)
{
  return (u32)(x & p::LowMask);
}

static constexpr size_t dword_combine2(u32 a, u32 b)
{
  size_t h = (size_t)a << p::HighShift,
    l = b;

  return h|l;
}
