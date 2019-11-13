#include <os/cpuid.h>
#include <os/panic.h>

#include <config>

#include <win32/cpuid.h>
#include <sysv/cpuid.h>

#include <cstdio>

namespace os {

CpuId cpuid()
{
#if __win32
  return win32::cpuid();
#elif __sysv
  return sysv::cpuid();
#else
#  error "unknown platform"
#endif
}

static thread_local char p_buf[256];
int cpuid_to_str(const CpuId& cpu, char *buf, size_t buf_sz)
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
