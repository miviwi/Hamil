#include <win32/win32.h>
#include <win32/cpuid.h>
#include <win32/time.h>
#include <win32/stdstream.h>

namespace win32 {

void init()
{
  check_sse_sse2_support();

  Timers::init();
  StdStream::init();
}

void finalize()
{
  StdStream::finalize();
}

}