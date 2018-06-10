#include <win32/stdstream.h>

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

#include <cstring>

namespace win32 {

int p_fds[2];
static char p_buf[4096];

void StdStream::init()
{
  _pipe(p_fds, sizeof(p_buf), O_TEXT);
  _dup2(p_fds[1], 1); // 1 == stdout
}

std::string StdStream::gets()
{
  printf("\r\n"); // Make sure there's something in the buffer so we don't block on _read()
  _flushall();
  int length = _read(p_fds[0], p_buf, sizeof(p_buf)-1);

  return std::string(p_buf, length-2); // Remove the superfluous CRLF added above
}

}