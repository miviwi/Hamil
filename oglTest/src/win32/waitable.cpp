#include <win32/waitable.h>

#include <Windows.h>

namespace win32 {

WaitResult Waitable::wait(ulong timeout)
{
  return (WaitResult)WaitForSingleObject(handle(), timeout);
}

}