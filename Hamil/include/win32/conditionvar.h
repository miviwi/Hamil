#pragma once

#include <os/conditionvar.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace os {
// Forward declaration
class Mutex;
}

namespace win32 {

class ConditionVariable final : public os::ConditionVariable {
public:
  ConditionVariable();
  ConditionVariable(ConditionVariable&& other);

  ConditionVariable& operator=(ConditionVariable&& other);

  virtual bool sleep(os::Mutex& mutex, ulong timeout_ms) final;
  
  virtual os::ConditionVariable& wake() final;
  virtual os::ConditionVariable& wakeAll() final;

private:
#if __win32
  CONDITION_VARIABLE m_cv;
#else
  void *m_cv;   // Need a dummy member for ConditionVariable(ConditionVariable&&), when !__win32
#endif
};

}
