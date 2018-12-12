#pragma once

#include <gx/gx.h>

#include <util/ref.h>

namespace gx {

class Fence : public Ref {
public:
  enum Status {
    Invalid,

    Signaled,
    Timeout,
  };

  enum : u64 {
    TimeoutInfinite = GL_TIMEOUT_IGNORED,
  };

  struct Error { };

  struct WaitFailedError : public Error { };

  Fence();
  ~Fence();

  // Inserts the Fence into the command stream
  //   - Can be called multiple times
  //   - When called more than once the old
  //     fence is discarded and a new one is
  //     created
  Fence& sync();

  // Returns 'true' if the fence had already
  //   been signaled
  bool signaled();

  // Blocks execution until either the Fence is
  //   signaled or 'timeout' expires
  Status block(u64 timeout = TimeoutInfinite);

  // Causes the DRIVER to wait for the Fence's
  //   completion before submitting commands
  //   that follow the call
  void wait();

  // Dummy method for ResourcePool
  void label(const char *lbl);

private:
  void /* GLsync */ *m;
  const char *m_label;

#if !defined(NDEBUG)
  bool m_waited = true;
#endif
};

}