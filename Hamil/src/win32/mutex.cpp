#include <win32/mutex.h>

#include <Windows.h>

namespace win32 {

Mutex::Mutex()
{
  InitializeCriticalSection(&m);
}

Mutex::~Mutex()
{
  DeleteCriticalSection(&m);
}

Mutex& Mutex::acquire()
{
  EnterCriticalSection(&m);

  return *this;
}

bool Mutex::tryAcquire()
{
  auto succeeded = TryEnterCriticalSection(&m);

  return succeeded == TRUE;
}

LockGuard<Mutex> Mutex::acquireScoped()
{
  return LockGuard<Mutex>(acquire());
}

Mutex& Mutex::release()
{
  LeaveCriticalSection(&m);

  return *this;
}

}