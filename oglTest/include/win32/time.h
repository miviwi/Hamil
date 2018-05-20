#pragma once

#include <common.h>

namespace win32 {

using Time = u64;

enum : Time {
  InvalidTime = ~0ull,
};

class Timers {
public:
  static void init();
  static void finalize();

  // Must be run continually (eg. every frame)
  static void tick();

  static Time ticks();

  static double timef_s();

  static Time time_s();
  static Time time_ms();
  static Time time_us();

  static Time ticks_per_s();

  static Time ticks_to_s(Time ticks);
  static Time ticks_to_ms(Time ticks);
  static Time ticks_to_us(Time ticks);

  static Time s_to_ticks(Time secs);
  static Time ms_to_ticks(Time msecs);
  static Time us_to_ticks(Time usecs);

  static Time s_to_ticks(double secs);
};

class Timer {
public:
  Timer();

  void reset();

protected:
  Time delta();

  Time m_started;
};

// Gives the amount of time elapsed since reset()
class DeltaTimer : public Timer {
public:
  Time elapsedSeconds();
  Time elapsedMillieconds();
  Time elapsedUseconds();

  double elapsedSecondsf();
};

// elapsedf():
//   Gives a normalized value <0; 1> of the duration elapsed since reset()
// duration<Milli,U>Seconds():
//   sets the duration
class DurationTimer : public Timer {
public:
  DurationTimer();

  DurationTimer& durationSeconds(Time duration);
  DurationTimer& durationMilliseconds(Time duration);
  DurationTimer& durationUseconds(Time duration);

  DurationTimer& durationSeconds(double duration);

  bool elapsed();
  float elapsedf();

  void clearDuration();

protected:
  Time m_duration;
};

// elapsedf():
//   Gives a normalized value <0; 1> of the duration elapsed since reset()
// elapedLoopsf():
//   the whole part is the number of loops since reset()
//   and the fractional part is the same as elapsedf()
class LoopTimer : public DurationTimer {
public:
  LoopTimer();

  LoopTimer& durationSeconds(Time duration);
  LoopTimer& durationMilliseconds(Time duration);
  LoopTimer& durationUseconds(Time duration);

  LoopTimer& durationSeconds(double duration);

  u64 loops();
  float elapsedf();

  float elapsedLoopsf();

private:
  double tick();

  u64 m_loops;
};

}
