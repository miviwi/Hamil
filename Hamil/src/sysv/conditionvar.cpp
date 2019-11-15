#include <sysv/conditionvar.h>
#include <sysv/mutex.h>
#include <os/time.h>
#include <os/waitable.h>

#include <config>

#include <cassert>

#if __sysv
#  include <time.h>
#endif

namespace sysv {

ConditionVariable::ConditionVariable()
{
#if __sysv
  auto error = pthread_cond_init(&m, nullptr);
  assert(!error && "failed to initialize sysv::ConditionVariable!");
#endif
}

ConditionVariable::~ConditionVariable()
{
#if __sysv
  auto error = pthread_cond_destroy(&m);
  assert(!error && "failed to destroy sysv::ConditionVariable!");
#endif
}

bool ConditionVariable::sleep(os::Mutex& mutex_, ulong timeout_ms)
{
#if __sysv
  auto& mutex = (sysv::Mutex&)mutex_;

  int error = -1;
  if(timeout_ms == os::WaitInfinite) {
    error = pthread_cond_wait(&m, &mutex.m);
  } else {
    // See sysv::Thread::wait() for notes on this code
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    struct timespec abstime;

    auto timeout_secs = timeout_ms / os::Timers::s_to_ms;
    auto timeout_nsecs = (timeout_ms % 1'000) * /* ms -> ns */ 1'000'000;

    abstime.tv_sec  = now.tv_sec + (time_t)timeout_secs;
    abstime.tv_nsec = now.tv_nsec + (long)timeout_nsecs;

    error = pthread_cond_timedwait(&m, &mutex.m, &abstime);
  }

  return !error;
#else
  return false;
#endif
}

os::ConditionVariable& ConditionVariable::wake()
{
#if __sysv
  auto error = pthread_cond_signal(&m);
  assert(!error && "sysv::ConditionVariable::wake() failed!");
#endif

  return *this;
}

os::ConditionVariable& ConditionVariable::wakeAll()
{
#if __sysv
  auto error = pthread_cond_broadcast(&m);
  assert(!error && "sysv::ConditionVariable::wakeAll() failed!");
#endif

  return *this;
}

}
