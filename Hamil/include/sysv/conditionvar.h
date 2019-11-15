#pragma once

#include <os/conditionvar.h>

#include <config>

#if __sysv
#  include <pthread.h>
#endif

namespace os {
// Forward declaration
class Mutex;
}

namespace sysv {

class ConditionVariable final : public os::ConditionVariable {
public:
  ConditionVariable();
  virtual ~ConditionVariable();

  virtual bool sleep(os::Mutex& mutex, ulong timeout_ms) final;

  virtual os::ConditionVariable& wake() final;
  virtual os::ConditionVariable& wakeAll() final;

private:
#if __sysv
  pthread_cond_t m;
#endif
};

}
