#pragma once

#include <common.h>

#if !defined(__linux__)
#  include <Windows.h>
#endif

namespace win32 {

class ConditionVariable;

template <typename T>
class LockGuard;

class Mutex {
public:
  Mutex();
  Mutex(const Mutex& other) = delete;
  ~Mutex();

  Mutex& acquire();
  bool tryAcquire();

  LockGuard<Mutex> acquireScoped();

  Mutex& release();

private:
  friend ConditionVariable;

#if !defined(__linux__)
  CRITICAL_SECTION m;
#endif
};

template <typename T>
class LockGuard {
public:
  LockGuard(T& lock) : 
    m_lock(lock)
  {
  }
  ~LockGuard()
  {
    m_lock.release();
  }

protected:

private:
  friend Mutex;

  T& m_lock;
};

}
