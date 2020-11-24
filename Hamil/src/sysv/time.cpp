#include <sysv/time.h>

#include <config>

#if __sysv
#  include <unistd.h>
#  include <time.h>
#endif

#include <numeric>
#include <array>

namespace sysv {

alignas(u64) volatile u64 p_ticks;

u64 p_ticks_per_s;

// Integer divide x and y rounding UP
static u64 ceil_div(u64 x, u64 y);

// Returns the value of the "time stamp counter" MSR (called the TSC for short)
static u64 rdtsc();
// Returns the number of TSC ticks in 10us (microseconds)
//   - calculated using the usleep() call
static u64 calibrate_tsc_ticks_in_10us();

void Timers::init()
{
  p_ticks_per_s = calibrate_tsc_ticks_in_10us() * (os::Timers::s_to_us / (Time)10);

  tick();
}

void Timers::finalize()
{
}

static u64 ceil_div(u64 x, u64 y)
{
  return 1+((x-1) / y);
}

static u64 rdtsc()
{
  u64 time;

  asm volatile(    /* mark as 'volatile' to ensure the assembly is actually emitted on every call */
      "xorq %%rax, %%rax\n\t"     /* make sure upper half of 'rax' is 0 (as only lower will be overwritten) */
      "rdtsc\n\t"
      "shlq $32, %%rdx\n\t"
      "orq %%rdx, %%rax\n\t"      /* merge edx:eax into a single quadword (stored in rax) */
      "movq %%rax, %0\n\t"        /* return the merged tsc value */

    : "=r"(time)      /* output */
    :                 /* input */
    : "%rax", "%rdx"  /* clobbered by 'rdtsc', upper halves used as temporaries */
  );

  return time;
}

static u64 calibrate_tsc_ticks_in_10us()
{
  static constexpr size_t NumCalibrationRuns = 4;
  static constexpr useconds_t CalibrationRunDurationUs = 10'000;    /* 10 milliseconds */

  std::array<uint64_t, NumCalibrationRuns> cal_runs;

  for(auto& run : cal_runs) {
    volatile uint64_t before = rdtsc();
    auto usleep_result = usleep(CalibrationRunDurationUs);
    volatile uint64_t after = rdtsc();

    // Put the assert() AFTER the rdtsc() calls to increase accuracy
    assert(usleep_result >= 0);

    uint64_t dt = after - before;

    run = ceil_div(dt, CalibrationRunDurationUs/10);   // Convert into units of 10us
  }

  uint64_t cal_runs_sum  = std::accumulate(cal_runs.begin(), cal_runs.end(), 0);
  uint64_t cal_runs_mean = ceil_div(cal_runs_sum, NumCalibrationRuns);

  return cal_runs_mean;   // Return the average of all the runs
}

void Timers::tick()
{
  p_ticks = rdtsc();
}

Time Timers::ticks()
{
  return (Time)p_ticks;
}

Time Timers::ticks_per_s()
{
  return (Time)p_ticks_per_s;
}

}
