#include <win32/win32.h>
#include <win32/cpuid.h>
#include <win32/cpuinfo.h>
#include <win32/time.h>
#include <win32/stdstream.h>

#include <os/cpuid.h>
#include <os/cpuinfo.h>

#include <xmmintrin.h>

namespace win32 {

void init()
{
#if !defined(NO_SSE)
  os::check_sse_sse2_support();
#if !defined(NO_AVX)
  os::check_avx_support();
#endif

  // Flush denormalized floats to 0
  _mm_setcsr(_mm_getcsr() | 0x8040);
#endif

  Timers::init();
}

void finalize()
{
}

}
