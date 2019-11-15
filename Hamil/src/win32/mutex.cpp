#include <win32/mutex.h>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

Mutex::Mutex()
{
#if __win32
  InitializeCriticalSection(&m);
#endif
}

Mutex::~Mutex()
{
#if __win32
  DeleteCriticalSection(&m);
#endif
}

os::Mutex& Mutex::acquire()
{
#if __win32
  EnterCriticalSection(&m);
#endif

  return *this;
}

bool Mutex::tryAcquire()
{
#if __win32
  auto succeeded = TryEnterCriticalSection(&m);

  return succeeded == TRUE;
#else
  return false;
#endif
}

os::Mutex& Mutex::release()
{
#if __win32
  LeaveCriticalSection(&m);
#endif

  return *this;
}

}
