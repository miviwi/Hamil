#include <os/panic.h>

#include <config>

#include <win32/panic.h>
#include <sysv/panic.h>

namespace os {

void panic(const char *reason, int exit_code)
{
#if __win32
  win32::panic(reason, exit_code);
#elif __sysv
  sysv::panic(reason, exit_code);
#else
#  error "unknown platform"
#endif
}

}
