#pragma once

#include <common.h>

#include <Windows.h>

namespace win32 {

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
  CRITICAL_SECTION m;
};

template <typename T>
class LockGuard {
public:
  LockGuard(T& lock) : 
    m_lock(lock)
  {
  }
  LockGuard(const LockGuard& other) = delete;
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