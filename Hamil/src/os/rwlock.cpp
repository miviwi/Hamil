#include <os/rwlock.h>

#include <win32/rwlock.h>
#include <sysv/rwlock.h>

#include <config>

#include <cassert>

namespace os {

ReaderWriterLock::Ptr ReaderWriterLock::alloc()
{
#if __win32
  return std::make_unique<win32::ReaderWriterLock>();
#elif __sysv
  return std::make_unique<sysv::ReaderWriterLock>();
#else
#  error "unknown platform"
#endif
}

}
