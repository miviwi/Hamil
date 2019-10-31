#include <win32/panic.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

namespace win32 {

void panic(const char *reason, int exit_code)
{
#if defined(_MSVC_VER)
  MessageBoxA(nullptr, reason, "Fatal Error", MB_OK|MB_ICONERROR);
  ExitProcess(exit_code);
#endif
}

}
