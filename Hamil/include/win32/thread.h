#pragma once

#include <common.h>
#include <os/thread.h>

#include <functional>

namespace win32 {

class ThreadData;

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

  virtual os::WaitResult wait(ulong timeout = os::WaitInfinite) final;

protected:
  virtual void doCreate(Fn fn, bool suspended) final;

private:
  static ulong ThreadProcTrampoline(void *param);

  ThreadData& data();
  const ThreadData& data() const;
};

}
