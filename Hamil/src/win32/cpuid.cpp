#include <win32/cpuid.h>
#include <win32/panic.h>

#include <intrin.h>

#include <cstdio>
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

  CpuIdF16CBit  = 29,
};

CpuInfo cpuid()
{
  CpuInfo cpu;

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

  cpu.f16c  = (ecx >> CpuIdF16CBit) & 1;

  return cpu;
}

static char p_buf[256];
int cpuid_to_str(const CpuInfo& cpu, char *buf, size_t buf_sz)
{
  auto yesno = [](int b) {
    return b ? "yes" : "no";
  };

  int sz = snprintf(buf ? buf : p_buf, buf ? buf_sz : sizeof(p_buf),
    "Vendor: %s\n"
    "\n"
    "RTDSC:  %s\n"
    "\n"
    "SSE:    %s\n"
    "SSE2:   %s\n"
    "SSE3:   %s\n"
    "SSSE3:  %s\n"
    "SSE41:  %s\n"
    "SSE42:  %s\n"
    "AVX:    %s\n"
    "FMA:    %s\n"
    "\n"
    "F16C:   %s",
    cpu.vendor,

    yesno(cpu.rdtsc),

    yesno(cpu.sse), yesno(cpu.sse2), yesno(cpu.sse3), yesno(cpu.ssse3),
    yesno(cpu.sse41), yesno(cpu.sse42), yesno(cpu.avx), yesno(cpu.fma),

    yesno(cpu.f16c));

  return sz + 1; // include '\0' in the size
}

void check_sse_sse2_support()
{
  auto cpu = cpuid();

  if(cpu.rdtsc &&  // RDTSC is almost certainly supported but check just to be sure :)
    cpu.sse && cpu.sse2 && cpu.sse3) return;

  panic("Your CPU doesn't support the required SSE extensions (SSE, SSE2, SSE3)", NoSSESupportError);
}

void check_avx_support()
{
  auto cpu = cpuid();

  if(cpu.avx) return;

  panic("Your CPU doesn't support the AVX extensions", NoAVXSupportError);
}

}