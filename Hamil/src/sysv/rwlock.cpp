#include <sysv/rwlock.h>

#include <config>

#include <cassert>
#include <cerrno>

namespace sysv {

ReaderWriterLock::ReaderWriterLock()
{
  auto error = pthread_rwlock_init(&m, nullptr);
  assert(!error && "failed to initialize sysv::ReaderWriterLock!");
}

ReaderWriterLock::~ReaderWriterLock()
{
  auto error = pthread_rwlock_destroy(&m);
  assert(!error && "failed to destroy sysv::ReaderWriterLock!");
}

os::ReaderWriterLock& ReaderWriterLock::acquireExclusive()
{
  auto error = pthread_rwlock_wrlock(&m);
  assert(!error && "sysv::ReaderWriterLock::acquireExclusive() failed!");

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::acquireShared()
{
  auto error = pthread_rwlock_rdlock(&m);
  assert(!error && "sysv::ReaderWriterLock::acquireShared() failed!");

  return *this;
}

bool ReaderWriterLock::tryAcquireExclusive()
{
  auto error = pthread_rwlock_trywrlock(&m);
  if(error == EBUSY) {
    return false;    // Another thread has the exclusive/shared lock
  }

  assert(!error && "sysv::ReaderWriterLock::tryAcquireExclusive() failed!");

  return true;
}

bool ReaderWriterLock::tryAcquireShared()
{
  auto error = pthread_rwlock_tryrdlock(&m);
  if(error == EBUSY) {
    return false;    // Another thread has the exclusive/shared lock
  }

  assert(!error && "sysv::ReaderWriterLock::tryAcquireShared() failed!");

  return true;
}

os::ReaderWriterLock& ReaderWriterLock::releaseExclusive()
{
  auto error = pthread_rwlock_unlock(&m);
  assert(!error && "sysv::ReaderWriterLock::releaseExclusive() failed!");

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::releaseShared()
{
  auto error = pthread_rwlock_unlock(&m);
  assert(!error && "sysv::ReaderWriterLock::releaseShared() failed!");

  return *this;
}

}
