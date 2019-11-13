#include <win32/cpuid.h>
#include <win32/panic.h>

#include <config>

#if __win32
#  include <intrin.h>
#endif

#include <cstring>

namespace win32 {

enum : int {
  CpuIdVendor   = 0,
  CpuIdFeatures = 1,

  CpuIdTSCBit   = 4,

  CpuIdSSEBit   = 25,
  CpuIdSSE2Bit  = 26,
  CpuIdSSE3Bit  = 0,
  CpuIdSSSE3Bit = 9,
  CpuIdSSE41Bit = 19,
  CpuIdSSE42Bit = 20,
  CpuIdAVXBit   = 28,
  CpuIdFMABit   = 12,

  CpuIdPOPCNTBit = 23,
  CpuIdF16CBit   = 29,
};

os::CpuId cpuid()
{
  os::CpuId cpu;

#if __win32
  union {
    int cpuid[4];
    struct {
      int eax, ebx, ecx, edx;
    };

    byte cpuid_raw[sizeof(int) * 4];
  };

  __cpuid(cpuid, CpuIdVendor);

  int vendor[4] = {
    ebx, edx, ecx, 0
  };
  memcpy(cpu.vendor, vendor, sizeof(cpu.vendor));

  __cpuid(cpuid, CpuIdFeatures);

  cpu.rdtsc = (edx >> CpuIdTSCBit) & 1;

  cpu.sse   = (edx >> CpuIdSSEBit)   & 1;
  cpu.sse2  = (edx >> CpuIdSSE2Bit)  & 1;
  cpu.sse3  = (ecx >> CpuIdSSE3Bit)  & 1;
  cpu.ssse3 = (ecx >> CpuIdSSSE3Bit) & 1;
  cpu.sse41 = (ecx >> CpuIdSSE41Bit) & 1;
  cpu.sse42 = (ecx >> CpuIdSSE42Bit) & 1;
  cpu.avx   = (ecx >> CpuIdAVXBit)   & 1;
  cpu.fma   = (ecx >> CpuIdFMABit)   & 1;

  cpu.popcnt = (ecx >> CpuIdPOPCNTBit) & 1;
  cpu.f16c   = (ecx >> CpuIdF16CBit)   & 1;
#endif

  return cpu;
}


}
