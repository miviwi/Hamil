#include <win32/rwlock.h>

#include <config>

namespace win32 {

ReaderWriterLock::ReaderWriterLock()
{
#if __win32
  InitializeSRWLock(&m);
#endif
}

os::ReaderWriterLock& ReaderWriterLock::acquireExclusive()
{
#if __win32
  AcquireSRWLockExclusive(&m);
#endif

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::acquireShared()
{
#if __win32
  AcquireSRWLockShared(&m);
#endif

  return *this;
}

bool ReaderWriterLock::tryAcquireExclusive()
{
#if __win32
  return TryAcquireSRWLockExclusive(&m) == TRUE;
#else
  return false;
#endif
}

bool ReaderWriterLock::tryAcquireShared()
{
#if __win32
  return TryAcquireSRWLockShared(&m) == TRUE;
#else
  return false;
#endif
}

os::ReaderWriterLock& ReaderWriterLock::releaseExclusive()
{
#if __win32
  ReleaseSRWLockExclusive(&m);
#endif

  return *this;
}

os::ReaderWriterLock& ReaderWriterLock::releaseShared()
{
#if __win32
  ReleaseSRWLockShared(&m);
#endif

  return *this;
}

}
