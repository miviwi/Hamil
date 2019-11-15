#include <sysv/rwlock.h>

#include <config>

#include <cassert>

namespace sysv {

ReaderWriterLock::ReaderWriterLock()
{
#if __sysv
  auto error = pthread_rwlock_init(&m, nullptr);
  assert(!error && "failed to initialize sysv::ReaderWriterLock!");
#endif
}

ReaderWriterLock::~ReaderWriterLock()
{
#if __sysv
  auto error = pthread_rwlock_destroy(&m);
  assert(!error && "failed to destroy sysv::ReaderWriterLock!");
#endif
}

os::ReaderWriterLock& ReaderWriterLock::acquireExclusive()
{
#if __sysv
  auto error = pthread_rwlock_wrlock(&m);
  assert(!error && "sysv::ReaderWriterLock::acquireExclusive() failed!");
#endif

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::acquireShared()
{
#if __sysv
  auto error = pthread_rwlock_rdlock(&m);
  assert(!error && "sysv::ReaderWriterLock::acquireShared() failed!");
#endif

  return *this;
}

bool ReaderWriterLock::tryAcquireExclusive()
{
#if __sysv
  auto error = pthread_rwlock_trywrlock(&m);
  if(error == EBUSY) {
    return false;    // Another thread has the exclusive/shared lock
  }

  assert(!error && "sysv::ReaderWriterLock::tryAcquireExclusive() failed!");

  return true;
#else
  return false;
#endif
}

bool ReaderWriterLock::tryAcquireShared()
{
#if __sysv
  auto error = pthread_rwlock_tryrdlock(&m);
  if(error == EBUSY) {
    return false;    // Another thread has the exclusive/shared lock
  }

  assert(!error && "sysv::ReaderWriterLock::tryAcquireShared() failed!");

  return true;
#else
  return false;
#endif
}

os::ReaderWriterLock& ReaderWriterLock::releaseExclusive()
{
#if __sysv
  auto error = pthread_rwlock_unlock(&m);
  assert(!error && "sysv::ReaderWriterLock::releaseExclusive() failed!");
#endif

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::releaseShared()
{
#if __sysv
  auto error = pthread_rwlock_unlock(&m);
  assert(!error && "sysv::ReaderWriterLock::releaseShared() failed!");
#endif

  return *this;
}

}
