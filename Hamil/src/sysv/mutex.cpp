#include <sysv/mutex.h>

#include <cassert>

namespace sysv {

Mutex::Mutex()
{
  auto error = pthread_mutex_init(&m, nullptr);
  assert(!error && "failed to initialize sysv::Mutex!");
}

Mutex::~Mutex()
{
  auto error = pthread_mutex_destroy(&m);
  assert(!error && "failed to destroy sysv::Mutex!");
}

os::Mutex& Mutex::acquire()
{
  auto error = pthread_mutex_lock(&m);
  assert(!error && "sysv::Mutex::acquire() failed!");

  return *this;
}

bool Mutex::tryAcquire()
{
  auto error = pthread_mutex_trylock(&m);
  if(error == EBUSY) {
    return false;     // The mutex was locked by another thread
  }

  assert(!error && "sysv::Mutex::tryAcquire() failed!");  // We handle all the allowable error above,
                                                          //   so if we have reached this point that
                                                          //   means there should be no more errors
  return true;
}

os::Mutex& Mutex::release()
{
  auto error = pthread_mutex_unlock(&m);
  assert(!error && "sysv::Mutex::release() failed!");

  return *this;
}

}
