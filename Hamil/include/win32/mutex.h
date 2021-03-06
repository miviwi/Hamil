#pragma once

#include <os/mutex.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

// Forward declaration
class ConditionVariable;

class Mutex final : public os::Mutex {
public:
  Mutex();
  virtual ~Mutex();

  virtual os::Mutex& acquire() final;
  virtual bool tryAcquire() final;

  virtual os::Mutex& release() final;

private:
  friend ConditionVariable;

#if __win32
  CRITICAL_SECTION m;
#endif
};

}
