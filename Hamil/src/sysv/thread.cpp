#include <sysv/thread.h>
#include <sysv/sysv.h>
#include <os/time.h>

#include <config>

#include <new>
#include <atomic>
#include <algorithm>

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>

// Linux
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

namespace sysv {

struct ThreadData {
  pthread_t self;

  std::atomic<bool> terminated;
  pthread_mutex_t terminate_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;

  std::atomic<u32> suspend_resume_ack;   // Filled with the id() of the Thread which
                                         //   raised the ThreadSuspendResumeSignal
                                         //   when it has been serviced
  pthread_mutex_t suspend_resume_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t suspend_resume_cond = PTHREAD_COND_INITIALIZER;

  Thread::Id id = Thread::InvalidId;

  Thread::Fn fn;

  // Incremented each time Thread::suspend() is called and
  //   desremented for every Thread::resume() call
  unsigned suspended;

  u32 exit_code = Thread::StillActive;

  void cleanup()
  {
    destroy_cond(&terminate_cond, &terminate_cond_mutex);
    destroy_cond(&suspend_resume_cond, &suspend_resume_cond_mutex);
  }

private:
  static void destroy_cond(pthread_cond_t *cond, pthread_mutex_t *cond_mutex)
  {
    auto cond_destroy_error = pthread_cond_destroy(cond);
    assert(!cond_destroy_error);

    auto cond_mutex_destroy_error = pthread_mutex_destroy(cond_mutex);
    assert(!cond_mutex_destroy_error);
  }
};

void *Thread::thread_proc_trampoline(void *arg)
{
  using thread_detail::ThreadSuspendResumeAction;

  auto self = (ThreadData *)arg;

  // Store this thread's id right away as the one which
  //   spawned it is waiting for this to happen
  self->id = (Id)gettid();

  if(self->suspended) {                           // If the thread was created with suspended=true
    auto action = wait_suspend_resume_signal();   //   wait for a resume signal before runnig any code
    thread_suspend_resume(action);
  }

  // Actually execute the code
  self->exit_code = self->fn();

  // Notify all threads waiting in Thread::wait()
  self->terminated.store(true);

  auto broadcast_error = pthread_cond_broadcast(&self->terminate_cond);
  assert(!broadcast_error);

  return nullptr;
}

thread_detail::ThreadSuspendResumeAction *Thread::wait_suspend_resume_signal()
{
  using thread_detail::ThreadSuspendResumeAction;

  sigset_t set;

  auto empty_error = sigemptyset(&set);
  assert(!empty_error);

  auto add_error = sigaddset(&set, thread_detail::ThreadSuspendResumeSignal);
  assert(!add_error);

  siginfo_t info;
  int wait_result = -1;
  do {
    wait_result = sigwaitinfo(&set, &info);      // Make sure we handle for ex. being
  } while(wait_result < 0 && errno == EINTR);    //   stopped by a debugger properly

  assert(wait_result == thread_detail::ThreadSuspendResumeSignal);

  return (ThreadSuspendResumeAction *)info.si_value.sival_ptr;
}

void Thread::thread_suspend_resume(thread_detail::ThreadSuspendResumeAction *action)
{
  using thread_detail::ThreadSuspendResumeAction;

  auto self = (ThreadData *)action->thread_data;

  switch(action->action) {
  case ThreadSuspendResumeAction::Resume:
    self->suspended--;
    break;

  case ThreadSuspendResumeAction::Suspend:
    self->suspended++;
    break;

  default: assert(0 && "invalid ThreadSuspendResumeAction::action specified!");
  }

  self->suspend_resume_ack.store(action->initiator);

  auto signal_error = pthread_cond_broadcast(&self->suspend_resume_cond);
  assert(!signal_error);

  // Wait for a resume signal before continuing if
  //   - we were just supended
  //   - we're still suspended
  while(self->suspended) {
    auto action = wait_suspend_resume_signal();
    
    return thread_suspend_resume(action);
  }
}

Thread::Thread() :
  os::Thread(sizeof(ThreadData))
{
  new(storage<ThreadData>()) ThreadData();
}

Thread::~Thread()
{
  // Only CHECK the ref-count so it's not decremented twice
  if(refs() > 1) return;

  assert(Thread::exitCode() != StillActive &&
      "at least ONE reference to a Thread object must be kept around while it's running!");

  if(id() != InvalidId) data().cleanup();

  // A count == 1 means somebody down the line will decrement
  //   it (i.e. set it to 0), thus the object is already dead
  //   and we need to clean it up
  storage<ThreadData>()->~ThreadData();
}

Thread::Id Thread::id() const
{
  return data().id;
}

os::Thread& Thread::dbg_SetName(const char *name)
{
#if !defined(NDEBUG)
  assert(id() != InvalidId &&    // make sure the Thread has has create() called on it
      "Thread must have had create() called on it before calling dbg_SetName()!");

  // Thread names can be at most 16 characters long (including the terminating '\0'),
  //   so trim the provided name so it meets this requirement
  size_t name_len = std::min<size_t>(strlen(name), 15);
  std::string trimmed_name(name, name_len);

  auto error = pthread_setname_np(data().self, trimmed_name.data());
  assert(!error);
#endif

  return *this;
}

os::Thread& Thread::resume()
{
  assert(id() != InvalidId && "Thread must've had create() called on it before resume()!");

  return raiseThreadSuspendResumeAction(thread_detail::ThreadSuspendResumeAction::Resume);
}

os::Thread& Thread::suspend()
{
  assert(id() != InvalidId && "Thread must've had create() called on it before suspend()!");

  return raiseThreadSuspendResumeAction(thread_detail::ThreadSuspendResumeAction::Suspend);
}

os::Thread& Thread::raiseThreadSuspendResumeAction(int action)
{
  using thread_detail::ThreadSuspendResumeAction;

  auto self_tid = gettid();
  auto& target = data().self;

  union sigval value;

  ThreadSuspendResumeAction suspend_resume_action;
  suspend_resume_action.initiator = self_tid;
  suspend_resume_action.action = action;
  suspend_resume_action.thread_data = storage();

  value.sival_ptr = &suspend_resume_action;
  
  auto sigqueue_error = pthread_sigqueue(target, thread_detail::ThreadSuspendResumeSignal, value);
  assert(!sigqueue_error);

  // Make sure the thread acknowledges the request before returning
  auto& suspend_resume_ack = data().suspend_resume_ack;
  u32 suspend_resume_ack_val = (u32)self_tid;

  auto cmpxchg = [&]() {
    return suspend_resume_ack.compare_exchange_strong(suspend_resume_ack_val, InvalidId);
  };

  while(!cmpxchg()) {  // Avoid spurious wakeups
    auto wait_error = pthread_cond_wait(
        &data().suspend_resume_cond, &data().suspend_resume_cond_mutex
    );
    assert(!wait_error);

    suspend_resume_ack_val = (u32)self_tid;   // Make sure the 'expected' value given to
  }                                           //   compare_exchange_strong() is what we want

  return *this;
}

os::Thread& Thread::affinity(uintptr_t mask)
{
  cpu_set_t cpus;
  CPU_ZERO(&cpus);

  for(unsigned cpu = 0; cpu < sizeof(mask)*CHAR_BIT; cpu++) {
    int cpu_active = mask&1;
    mask >>= 1;

    if(!cpu_active) continue;

    CPU_SET(cpu, &cpus);
  }

  auto result = sched_setaffinity(gettid(), sizeof(cpus), &cpus);
  if(result < 0) throw SetAffinityError();

  return *this;
}

u32 Thread::exitCode()
{
  assert(id() != InvalidId && "a Thread must've had create() called on it before calling exitCode()!");

  return data().exit_code;
}

os::WaitResult Thread::wait(ulong timeout_ms)
{
  assert(id() != InvalidId && "a Thread must've had create() called on it before calling wait()!");

  auto cond = &data().terminate_cond;
  auto cond_mutex = &data().terminate_cond_mutex;

  auto& terminated = data().terminated;
  
  int wait_error = -1;
  if(timeout_ms == os::WaitInfinite) {
    do {    // Avoid spurious wakeups
      wait_error = pthread_cond_wait(cond, cond_mutex);
      if(wait_error) break;
    } while(!terminated.load());
  } else {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    struct timespec abstime;

    auto timeout_secs = timeout_ms / os::Timers::s_to_ms;

    // Take only the millisecond part from 'timeout_ms' and
    //   convert them to nanoseconds
    auto timeout_nsecs = (timeout_ms % 1'000) * /* ms -> ns */ 1'000'000;

    // Convert the timeout duration to a wall clock end time
    abstime.tv_sec  = now.tv_sec + (time_t)timeout_secs;
    abstime.tv_nsec = now.tv_nsec + (long)timeout_nsecs;

    while(!terminated.load()) {    // Avoid spurious wakeups
      wait_error = pthread_cond_timedwait(cond, cond_mutex, &abstime);
      if(wait_error) {
        if (wait_error == ETIMEDOUT) return os::WaitTimeout;

        break;
      }
    }
  }

  // ETIMEDOUT -> os::WaitTimeout handled above
  if(wait_error) return os::WaitFailed;

  return os::WaitObject0;
}

void Thread::doCreate(Fn fn, bool suspended)
{
  pthread_attr_t attr;
  auto attr_init_error = pthread_attr_init(&attr);
  if(attr_init_error) throw CreateError();

  auto setdetachstate_error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if(setdetachstate_error) throw CreateError();

  data().suspend_resume_ack = Thread::InvalidId;
  data().terminated = false;

  data().fn = fn;
  data().suspended = suspended ? 1 : 0;

  auto create_error = pthread_create(
      &data().self, &attr,
      &thread_proc_trampoline, storage()
  );
  if(create_error) throw CreateError();

  do {
    sleep(0);    // Yield so thread_proc_trampoline() has a chance to fill data().id
  } while(data().id == InvalidId);
  
  auto attr_destroy_error = pthread_attr_destroy(&attr);
  assert(!attr_destroy_error);
}

ThreadData& Thread::data()
{
  return *storage<ThreadData>();
}

const ThreadData& Thread::data() const
{
  return *storage<ThreadData>();
}

}
