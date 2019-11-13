#include <os/time.h>

#include <win32/time.h>
#include <sysv/time.h>
#include <math/geometry.h>
#include <math/util.h>

#include <config>

#include <cmath>

namespace os {

void Timers::init()
{
#if __win32
  win32::Timers::init();
#elif __sysv
  sysv::Timers::init();
#else
#  error "unknown platform"
#endif
}

void Timers::finalize()
{
#if __win32
  win32::Timers::finalize();
#elif __sysv
  sysv::Timers::finalize();
#else
#  error "unknown platform"
#endif
}

void Timers::tick()
{
#if __win32
  win32::Timers::tick();
#elif __sysv
  sysv::Timers::tick();
#else
#  error "unknown platform"
#endif
}

Time Timers::ticks()
{
#if __win32
  return win32::Timers::ticks();
#elif __sysv
  return sysv::Timers::ticks();
#else
#  error "unknown platform"
#endif
}

double Timers::timef_s()
{
  return (double)ticks() / (double)ticks_per_s();
}

Time Timers::time_s()
{
  return ticks_to_s(ticks());
}

Time Timers::time_ms()
{
  return ticks_to_ms(ticks());
}

Time Timers::time_us()
{
  return ticks_to_us(ticks());
}

Time Timers::ticks_per_s()
{
#if __win32
  return win32::Timers::ticks_per_s();
#elif __sysv
  return sysv::Timers::ticks_per_s();
#else
#  error "unknown platform"
#endif
}

Time Timers::ticks_to_s(Time ticks)
{
  return ticks / ticks_per_s();
}

Time Timers::ticks_to_ms(Time ticks)
{
  return (ticks*s_to_ms) / ticks_per_s();
}

Time Timers::ticks_to_us(Time ticks)
{
  return (ticks*s_to_us) / ticks_per_s();
}

Time Timers::ticks_to_ns(Time ticks)
{
  return (ticks*s_to_ns) / ticks_per_s();
}

Time Timers::s_to_ticks(Time secs)
{
  return secs * ticks_per_s();
}

Time Timers::ms_to_ticks(Time msecs)
{
  return (msecs * ticks_per_s())/s_to_ms;
}

Time Timers::us_to_ticks(Time usecs)
{
  return (usecs * ticks_per_s())/s_to_us;
}

Time Timers::ns_to_ticks(Time nsecs)
{
  return (nsecs * ticks_per_s())/s_to_ns;
}

Time Timers::s_to_ticks(double secs)
{
  double t = (double)ticks_per_s() * secs;

  return (Time)t;
}

double Timers::ticks_to_sf(Time ticks)
{
  double t = (double)ticks / (double)ticks_per_s();

  return t;
}

Timer::Timer() :
  m_started(InvalidTime)
{
  reset();
}

void Timer::reset()
{
  m_started = Timers::ticks();
}

void Timer::stop()
{
  m_started = InvalidTime;
}

Time Timer::delta()
{
  auto dt = Timers::ticks() - m_started;
  return m_started != InvalidTime ? dt : 0;
}

Time DeltaTimer::elapsedTicks()
{
  return delta();
}

Time DeltaTimer::elapsedSeconds()
{
  return Timers::ticks_to_s(delta());
}

Time DeltaTimer::elapsedMillieconds()
{
  return Timers::ticks_to_ms(delta());
}

Time DeltaTimer::elapsedUseconds()
{
  return Timers::ticks_to_us(delta());
}

double DeltaTimer::elapsedSecondsf()
{
  auto dt = (double)delta();
  return dt/(double)Timers::ticks_per_s();
}

DurationTimer::DurationTimer() :
  m_duration(InvalidTime)
{
}

DurationTimer& DurationTimer::durationTicks(Time duration)
{
  m_duration = duration;

  return *this;
}

DurationTimer& DurationTimer::durationSeconds(Time duration)
{
  m_duration = Timers::s_to_ticks(duration);

  return *this;
}

DurationTimer& DurationTimer::durationMilliseconds(Time duration)
{
  m_duration = Timers::ms_to_ticks(duration);

  return *this;
}

DurationTimer& DurationTimer::durationUseconds(Time duration)
{
  m_duration = Timers::us_to_ticks(duration);

  return *this;
}

DurationTimer& DurationTimer::durationSeconds(double duration)
{
  double ticks_per_s = Timers::ticks_per_s();
  m_duration = (Time)(duration * ticks_per_s);

  return *this;
}

Time DurationTimer::duration() const
{
  return m_duration;
}

bool DurationTimer::elapsed()
{
  return m_duration != InvalidTime ? Timers::ticks() >= (m_started + m_duration) : false; 
}

float DurationTimer::elapsedf()
{
  double x = (double)delta() / (double)m_duration;
  return clamp((float)x, 0.0f, 1.0f);
}

void DurationTimer::clearDuration()
{
  m_duration = InvalidTime;
}

LoopTimer::LoopTimer() :
  m_loops(0)
{
}

LoopTimer& LoopTimer::durationTicks(Time duration)
{
  DurationTimer::durationTicks(duration);

  return *this;
}

LoopTimer& LoopTimer::durationSeconds(Time duration)
{
  DurationTimer::durationSeconds(duration);

  return *this;
}

LoopTimer& LoopTimer::durationMilliseconds(Time duration)
{
  DurationTimer::durationMilliseconds(duration);

  return *this;
}

LoopTimer& LoopTimer::durationUseconds(Time duration)
{
  DurationTimer::durationUseconds(duration);

  return *this;
}

LoopTimer& LoopTimer::durationSeconds(double duration)
{
  DurationTimer::durationSeconds(duration);

  return *this;
}

u64 LoopTimer::loops()
{
  tick();
  return m_loops;
}

float LoopTimer::elapsedf()
{
  return tick();
}

float LoopTimer::elapsedLoopsf()
{
  auto x = tick();
  return x + (float)m_loops;
}

double LoopTimer::tick()
{
  double loops = 0;
  double x = (double)delta() / (double)m_duration;

  x = modf(x, &loops);
  m_loops = loops;

  return x;
}

}
