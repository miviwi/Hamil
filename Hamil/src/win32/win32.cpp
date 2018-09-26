#include <win32/win32.h>
#include <win32/cpuid.h>
#include <win32/time.h>
#include <win32/stdstream.h>

#include <xmmintrin.h>

namespace win32 {

void init()
{
#if !defined(NO_SSE)
  check_sse_sse2_support();
#if !defined(NO_AVX)
  check_avx_support();
#endif

  // Flush denormalized floats to 0
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif


  Timers::init();
  StdStream::init();
}

void finalize()
{
  StdStream::finalize();
}

}