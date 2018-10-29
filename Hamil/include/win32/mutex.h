#pragma once

#include <common.h>

#include <Windows.h>

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

  CRITICAL_SECTION m;
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