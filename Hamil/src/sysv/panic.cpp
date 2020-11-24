#include <sysv/panic.h>

#include <config>

#if __sysv
#  include <unistd.h>
#endif

#include <cassert>
#include <cstdio>

#define XMESSAGE_EXIT_BUTTON_LABEL "Exit"

namespace sysv {

static char g_xmessage_button[256];

void panic(const char *reason, int exit_code)
{
  snprintf(g_xmessage_button, sizeof(g_xmessage_button),
      XMESSAGE_EXIT_BUTTON_LABEL ":%d", exit_code);

  execl("/bin/xmessage",
      "xmessage",
      "-button", g_xmessage_button,
      "-default", XMESSAGE_EXIT_BUTTON_LABEL,
      reason,
      (char *)nullptr
  );
}

}
