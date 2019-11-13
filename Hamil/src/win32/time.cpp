#include <win32/time.h>
#include <win32/panic.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

#if __win32
static LARGE_INTEGER p_perf_freq = { 0 };
static LARGE_INTEGER p_perf_counter = { 0 };
#endif

alignas(i64) volatile i64 p_ticks;

void Timers::init()
{
#if __win32
  auto result = QueryPerformanceFrequency(&p_perf_freq);
  if(!result) panic("QueryPerformanceCounter() failed!", QueryPerformanceCounterError);

  tick();
#endif
}

void Timers::finalize()
{
}

#if __win32

void Timers::tick()
{
  QueryPerformanceCounter(&p_perf_counter);

  InterlockedExchange64(&p_ticks, p_perf_counter.QuadPart);
}

Time Timers::ticks()
{
  return (Time)p_ticks; // 'p_ticks' is declared as volatile and properly
                        //   aligned which is enough for MSVC to guarantee 
                        //   atomicity of the read
}

Time Timers::ticks_per_s()
{
  return (Time)p_perf_freq.QuadPart; // The frequency never changes during runtime
}

#endif       // __win32

}
