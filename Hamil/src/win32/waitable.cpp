#include <win32/waitable.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

namespace win32 {

WaitResult Waitable::wait(ulong timeout)
{
#if defined(_MSVC_VER)
  return (WaitResult)WaitForSingleObject(handle(), timeout);
#else
  return WaitFailed;
#endif
}

}
