#pragma once

#include <common.h>
#include <os/mutex.h>
#include <os/waitable.h>

#include <memory>

namespace os {

class ConditionVariable {
public:
  using Ptr = std::unique_ptr<ConditionVariable>;

  static Ptr alloc();

  ConditionVariable() = default;
  ConditionVariable(const ConditionVariable&) = delete;
  virtual ~ConditionVariable() = default;

  virtual bool sleep(Mutex& mutex, ulong timeout_ms = WaitInfinite) = 0;

  // Alias for sleep(*mutex, timeout_ms)
  bool sleep(Mutex::Ptr& mutex, ulong timeout_ms = WaitInfinite);

  template <typename Pred>
  bool sleep(Mutex& mutex, Pred pred, ulong timeout_ms = WaitInfinite)
  {
    bool result = false;
    while(!pred()) result = sleep(mutex, timeout_ms);

    return result;
  }

  // Alias for sleep(*mutex, pred, timeout_ms)
  template <typename Pred>
  bool sleep(Mutex::Ptr& mutex, Pred pred, ulong timeout_ms = WaitInfinite)
  {
    return sleep(*mutex, pred, timeout_ms);
  }

  virtual ConditionVariable& wake() = 0;
  virtual ConditionVariable& wakeAll() = 0;
};

}
