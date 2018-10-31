#include <sched/job.h>

namespace sched {

win32::ConditionVariable& IJob::condition()
{
  return m_cv;
}

bool IJob::done()
{
  return m_done.load();
}

double IJob::dbg_ElapsedTime() const
{
#if !defined(NDEBUG)
  return m_dt;
#else
  return INFINITY;
#endif
}

void IJob::started()
{
#if !defined(NDEBUG)
  win32::Timers::tick();

  m_timer.reset();
  m_dt = 0.0;
#endif
}

void IJob::finished()
{
#if !defined(NDEBUG)
  win32::Timers::tick();
  m_dt = m_timer.elapsedSecondsf();
#endif

  m_done.store(true);
  m_cv.wakeAll();
}

}