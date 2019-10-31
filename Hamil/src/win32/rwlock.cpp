#include <win32/rwlock.h>

namespace win32 {

ReaderWriterLock::ReaderWriterLock()
{
#if !defined(__linux__)
  InitializeSRWLock(&m);
#endif
}

ReaderWriterLock::~ReaderWriterLock()
{
}

ReaderWriterLock& ReaderWriterLock::acquireExclusive()
{
#if !defined(__linux__)
  AcquireSRWLockExclusive(&m);
#endif

  return *this;
}

ReaderWriterLock& ReaderWriterLock::acquireShared()
{
#if !defined(__linux__)
  AcquireSRWLockShared(&m);
#endif

  return *this;
}

bool ReaderWriterLock::tryAcquireExclusive()
{
#if !defined(__linux__)
  return TryAcquireSRWLockExclusive(&m) == TRUE;
#else
  return false;
#endif
}

bool ReaderWriterLock::tryAcquireShared()
{
#if !defined(__linux__)
  return TryAcquireSRWLockShared(&m) == TRUE;
#else
  return false;
#endif
}

ReaderWriterLock& ReaderWriterLock::releaseExclusive()
{
#if !defined(__linux__)
  ReleaseSRWLockExclusive(&m);
#endif

  return *this;
}

ReaderWriterLock& ReaderWriterLock::releaseShared()
{
#if !defined(__linux__)
  ReleaseSRWLockShared(&m);
#endif

  return *this;
}

}
