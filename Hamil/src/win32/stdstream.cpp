#include <win32/stdstream.h>

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

#include <cstring>

namespace win32 {

int p_fds[2];

static constexpr size_t p_buf_sz = 1024*1024; // 1MB
static char *p_buf;

void StdStream::init()
{
  p_buf = new char[p_buf_sz];

  _pipe(p_fds, p_buf_sz, O_TEXT);
  _dup2(p_fds[1], 1); // 1 == stdout
}

void StdStream::finalize()
{
  delete[] p_buf;
}

std::string StdStream::gets()
{
  printf("\r\n"); // Make sure there's something in the buffer so we don't block on _read()
  _flushall();
  int length = _read(p_fds[0], p_buf, p_buf_sz-1);

  return std::string(p_buf, length-2); // Remove the superfluous CRLF added above
}

}