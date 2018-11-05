#pragma once

#include <common.h>

#include <win32/waitable.h>

#include <functional>

namespace win32 {

// PIMPL class
//   - Instances of it are always allocated on the heap
//     so things don't get messy with Thread objects stored
//     on the stack
//   - This unfortunately means each Thread must do 2 allocations
//     upon creation (one for the ref-count and another for 
//     ThreadData) but this overhead should be negligible
//     compared to actually calling CreateThread()
class ThreadData;

// A reference to a Thread MUST be kept around while it's running
//   or undefined behaviour WILL occur
// Threads may be created on the stack or stored in containers, moved and
//   copied around (treated just like primitive types) - because they are
//   ref-counted they will be disposed of only when there are no more
//   references to a given Thread remaining
class Thread : public Waitable {
public:
  using Id = ulong;
  using Fn = std::function<ulong()>;

  using OnExit = std::function<void()>;

  enum : Id {
    InvalidId = ~0u,
  };

  struct Error {
    const unsigned what;
    Error(unsigned what_) :
      what(what_)
    { }
  };

  struct CreateError : public Error {
    using Error::Error;
  };

  enum : ulong {
    // Returned by exitCode() when the Thread is still running
    StillActive = /* STILL_ACTIVE */ 259,
  };

  Thread(Fn fn, bool suspended = false);
  ~Thread();

  // Returns an Id unique to this Thread
  Id id() const;

  Thread& dbg_SetName(const char *name);

  // Returns the Id of the caller's (currently running) Thread
  static Id current_thread_id();

  Thread& resume();
  Thread& suspend();

  // Setting the i-th bit allows the Thread to
  //   run on the i-th thread (LSB first)
  Thread& affinity(uintptr_t mask);

  // See note for StillActive above
  ulong exitCode() const;

  // The code runs on the created Thread just BEFORE cleanup
  Thread& onExit(OnExit on_exit);

private:
  static ulong ThreadProcTrampoline(void *param);

  ThreadData *m_data;
};

}