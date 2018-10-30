#pragma once

#include <win32/win32.h>
#include <win32/waitable.h>
#include <win32/mutex.h>

#include <Windows.h>

namespace win32 {

class ConditionVariable {
public:
  ConditionVariable();
  ConditionVariable(const ConditionVariable& other) = delete;
  ~ConditionVariable();

  bool sleep(Mutex& mutex, ulong timeout = WaitInfinite);
  
  template <typename Pred>
  bool sleep(Mutex& mutex, Pred pred, ulong timeout = WaitInfinite)
  {
    bool result = false;
    while(!pred()) result = sleep(mutex, timeout);

    return result;
  }

  void wake();
  void wakeAll();

private:
  CONDITION_VARIABLE m_cv;
};

}