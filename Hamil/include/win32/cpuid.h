#pragma once

#include <common.h>

namespace win32 {

struct CpuId {
  char vendor[16];

  u32 rdtsc : 1;

  u32 sse   : 1;
  u32 sse2  : 1;
  u32 sse3  : 1;
  u32 ssse3 : 1;
  u32 sse41 : 1;
  u32 sse42 : 1;
  u32 avx   : 1;
  u32 fma   : 1;

  u32 popcnt : 1;
  u32 f16c   : 1;
};

CpuId cpuid();

// Writes a human-readable description of the CpuInfo structure 
//   info buffer 'buf' with 'buf_sz' bytes of capacity and returns
//   the number of bytes written (whole string is ~150 bytes long)
// 'buf' can be NULL in which case no write occurs and only the
//   required space in the buffer is returned (including '\0')
int cpuid_to_str(const CpuId& cpu, char *buf, size_t buf_sz);

void check_sse_sse2_support();
void check_avx_support();

}