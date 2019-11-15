#include <os/conditionvar.h>

#include <win32/conditionvar.h>
#include <sysv/conditionvar.h>

#include <config>

namespace os {

ConditionVariable::Ptr ConditionVariable::alloc()
{
#if __win32
  return std::make_unique<win32::ConditionVariable>();
#elif __sysv
  return std::make_unique<sysv::ConditionVariable>();
#else
#  error "unknown platform"
#endif
}

bool ConditionVariable::sleep(Mutex::Ptr& mutex, ulong timeout_ms)
{
  return sleep(*mutex, timeout_ms);
}

}
