#include <win32/panic.h>

#include <Windows.h>

namespace win32 {

void panic(const char *reason, int exit_code)
{
  MessageBoxA(nullptr, reason, "Fatal Error", MB_OK|MB_ICONERROR);
  ExitProcess(exit_code);
}

}