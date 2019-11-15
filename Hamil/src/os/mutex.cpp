#include <os/mutex.h>

#include <win32/mutex.h>
#include <sysv/mutex.h>

#include <config>

#include <cassert>

namespace os {

Mutex::Ptr Mutex::alloc()
{
#if __win32
  return std::make_unique<win32::Mutex>();
#elif __sysv
  return std::make_unique<sysv::Mutex>();
#else
#  error "unknown platform"
#endif
}

LockGuard<Mutex> Mutex::acquireScoped()
{
  return LockGuard<Mutex>(acquire());
}

}
