#include <win32/panic.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

void panic(const char *reason, int exit_code)
{
#if __win32
  MessageBoxA(nullptr, reason, "Fatal Error", MB_OK|MB_ICONERROR);
  ExitProcess(exit_code);
#endif
}

}
