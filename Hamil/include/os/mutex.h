#pragma once

#include <common.h>

#include <memory>

namespace os {

// Forward declaration
template <typename T>
class LockGuard;

class Mutex {
public:
  using Ptr = std::unique_ptr<Mutex>;

  static Ptr alloc();

  Mutex() = default;
  Mutex(const Mutex&) = delete;
  virtual ~Mutex() = default;

  virtual Mutex& acquire() = 0;
  virtual bool tryAcquire() = 0;

  LockGuard<Mutex> acquireScoped();

  virtual Mutex& release() = 0;
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

private:
  T& m_lock;
};

}
