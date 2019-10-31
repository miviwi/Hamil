#include <win32/mutex.h>

#if !defined(__linux__)
#  include <Windows.h>
#endif

namespace win32 {

Mutex::Mutex()
{
#if !defined(__linux__)
  InitializeCriticalSection(&m);
#endif
}

Mutex::~Mutex()
{
#if !defined(__linux__)
  DeleteCriticalSection(&m);
#endif
}

Mutex& Mutex::acquire()
{
#if !defined(__linux__)
  EnterCriticalSection(&m);
#endif

  return *this;
}

bool Mutex::tryAcquire()
{
#if !defined(__linux__)
  auto succeeded = TryEnterCriticalSection(&m);

  return succeeded == TRUE;
#else
  return false;
#endif
}

LockGuard<Mutex> Mutex::acquireScoped()
{
  return LockGuard<Mutex>(acquire());
}

Mutex& Mutex::release()
{
#if !defined(__linux__)
  LeaveCriticalSection(&m);
#endif

  return *this;
}

}
