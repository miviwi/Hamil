#include <win32/rwlock.h>

namespace win32 {

ReaderWriterLock::ReaderWriterLock()
{
  InitializeSRWLock(&m);
}

ReaderWriterLock::~ReaderWriterLock()
{
}

ReaderWriterLock& ReaderWriterLock::acquireExclusive()
{
  AcquireSRWLockExclusive(&m);

  return *this;
}

ReaderWriterLock& ReaderWriterLock::acquireShared()
{
  AcquireSRWLockShared(&m);

  return *this;
}

bool ReaderWriterLock::tryAcquireExclusive()
{
  return TryAcquireSRWLockExclusive(&m) == TRUE;
}

bool ReaderWriterLock::tryAcquireShared()
{
  return TryAcquireSRWLockShared(&m) == TRUE;
}

ReaderWriterLock& ReaderWriterLock::releaseExclusive()
{
  ReleaseSRWLockExclusive(&m);

  return *this;
}

ReaderWriterLock& ReaderWriterLock::releaseShared()
{
  ReleaseSRWLockShared(&m);

  return *this;
}

}