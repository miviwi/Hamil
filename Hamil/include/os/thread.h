#pragma once

#include <common.h>
#include <os/error.h>
#include <os/waitable.h>

#include <functional>

namespace os {

// PIMPL class
//   - Instances of it are always allocated on the heap
//     so things don't get messy with Thread objects stored
//     on the stack
//   - This unfortunately means each Thread must do 2 allocations
//     upon creation (one for the ref-count and another for 
//     ThreadData) but this overhead should be negligible
//     compared to actually calling CreateThread()
//   - It's size is extended by 'storage_sz' bytes to allow
//     storing os-specific data
struct ThreadData;

class Thread : public Waitable {
public:
  using Id = u32;
  using Fn = std::function<int()>;

  using OnExit = std::function<void()>;

  enum : Id {
    InvalidId = ~0u,
  };

  struct CreateError final : public Error {
    CreateError() :
      Error("an error occured during os::Thread creation")
    { }
  };

  struct SetAffinityError final : public Error {
    SetAffinityError() :
      Error("failed to set Thread's affinity()!")
    { }
  };

  enum ExitCode : u32 {
    Success = 0,
    StillActive = ~0u,
  };

  // Returns the Id of the caller's (currently running) Thread
  static Id current_thread_id();
  static Thread *alloc();

  virtual ~Thread();

  Thread& create(Fn fn, bool suspended = false);

  // Returns an Id unique to this Thread
  virtual Id id() const = 0;

  virtual Thread& dbg_SetName(const char *name) = 0;

  virtual Thread& resume() = 0;
  virtual Thread& suspend() = 0;

  // Setting the i-th bit allows the Thread to
  //   run on the i-th thread (LSB first)
  virtual Thread& affinity(uintptr_t mask) = 0;

  // See note for StillActive above
  virtual u32 exitCode() = 0;

  // The code runs on the created Thread just BEFORE cleanup
  Thread& onExit(OnExit on_exit);

protected:
  Thread(size_t storage_sz);

  void *storage();
  const void *storage() const;

  template <typename T>
  T *storage()
  {
    return (T *)storage();
  }
  template <typename T>
  const T *storage() const
  {
    return (const T *)storage();
  }

  // Called by create(), which stores the returned Id
  //   for later retrieval in ThreadData
  virtual void doCreate(Fn fn, bool suspended) = 0;

private:
  ThreadData *m_data;
};

}
