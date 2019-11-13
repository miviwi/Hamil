#include <win32/waitable.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

WaitResult Waitable::wait(ulong timeout)
{
#if __win32
  return (WaitResult)WaitForSingleObject(handle(), timeout);
#else
  return WaitFailed;
#endif
}

}
