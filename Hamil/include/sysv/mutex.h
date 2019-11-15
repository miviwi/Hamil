#pragma once

#include <os/mutex.h>

#include <config>

#if __syv
#  include <pthread.h>
#endif

namespace sysv {

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

#if __sysv
  pthread_mutex_t m;
#endif
};

}
