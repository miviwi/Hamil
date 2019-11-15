#include <sched/job.h>

#include <cmath>

#include <utility>

namespace sched {

IJob::IJob() :
  m_done(true)
{
  m_cv = os::ConditionVariable::alloc();
}

IJob::IJob(IJob&& other) :
  m_done(other.m_done.load()), m_cv(std::move(other.m_cv))
{
  // Make sure no deadlocks or other strange things occur
  //   when 'other' is used after this somehow
  other.m_done.store(true);
}

os::ConditionVariable::Ptr& IJob::condition()
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

void IJob::scheduled()
{
  m_done.store(false);
}

void IJob::started()
{
#if !defined(NDEBUG)
  os::Timers::tick();

  m_timer.reset();
  m_dt = 0.0;
#endif
}

void IJob::finished()
{
#if !defined(NDEBUG)
  os::Timers::tick();
  m_dt = m_timer.elapsedSecondsf();
#endif

  m_done.store(true);
  m_cv->wakeAll();
}

}
