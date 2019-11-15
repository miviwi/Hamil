#include <sysv/mutex.h>

#include <cassert>

namespace sysv {

Mutex::Mutex()
{
#if __sysv
  auto error = pthread_mutex_init(&m, nullptr);
  assert(!error && "failed to initialize sysv::Mutex!");
#endif
}

Mutex::~Mutex()
{
#if __sysv
  auto error = pthread_mutex_destroy(&m);
  assert(!error && "failed to destroy sysv::Mutex!");
#endif
}

os::Mutex& Mutex::acquire()
{
#if __sysv
  auto error = pthread_mutex_lock(&m);
  assert(!error && "sysv::Mutex::acquire() failed!");
#endif

  return *this;
}

bool Mutex::tryAcquire()
{
#if __sysv
  auto error = pthread_mutex_trylock(&m);
  if(error == EBUSY) {
    return false;     // The mutex was locked by another thread
  }

  assert(!error && "sysv::Mutex::tryAcquire() failed!");  // We handle all the allowable error above,
                                                          //   so if we have reached this point that
                                                          //   means there should be no more errors
  return true;
#else
  return false;
#endif
}

os::Mutex& Mutex::release()
{
#if __sysv
  auto error = pthread_mutex_unlock(&m);
  assert(!error && "sysv::Mutex::release() failed!");
#endif

  return *this;
}

}
