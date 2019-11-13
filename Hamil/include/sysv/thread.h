#pragma once

#include <os/thread.h>

namespace sysv::thread_detail {
// Forward declaration
struct ThreadSuspendResumeAction;
}

namespace sysv {

struct ThreadData;

// A reference to a Thread MUST be kept around while it's running
//   or undefined behaviour WILL occur
// Threads may be created on the stack or stored in containers, moved and
//   copied around (treated just like primitive types) - because they are
//   ref-counted they will be disposed of only when there are no more
//   references to a given Thread remaining
class Thread final : public os::Thread {
public:
  Thread();
  virtual ~Thread();

  virtual Id id() const final;

  virtual os::Thread& dbg_SetName(const char *name) final;

  virtual os::Thread& resume() final;
  virtual os::Thread& suspend() final;

  virtual os::Thread& affinity(uintptr_t mask) final;

  virtual u32 exitCode() final;

  virtual os::WaitResult wait(ulong timeout = os::WaitInfinite);

  // Can be called ONLY by thread_suspend_resume_sigaction()!
  static void thread_suspend_resume(thread_detail::ThreadSuspendResumeAction *action);

protected:
  virtual void doCreate(Fn fn, bool suspended) final;

private:
  static void *thread_proc_trampoline(void /* ThreadData */ *arg);

  // Waits for a thread_detail::ThreadSuspendResumeSignal and
  //   returns it's si_value as a ThreadSuspendResumeAction*
  static thread_detail::ThreadSuspendResumeAction *wait_suspend_resume_signal();

  os::Thread& raiseThreadSuspendResumeAction(int action);

  ThreadData& data();
  const ThreadData& data() const;
};

}
