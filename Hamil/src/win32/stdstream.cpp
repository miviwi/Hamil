#include <win32/stdstream.h>

#if __win32
#  include <Windows.h>
#  include <io.h>
#  include <fcntl.h>
#endif

#include <cstring>

namespace win32::stdstream_detail {

[[maybe_unused]] static int p_fds[2];

void init()
{
#if __win32
  _pipe(p_fds, p_buf_sz, O_TEXT);
  _dup2(p_fds[1], 1 /* stdout */);
#endif
}

void finalize()
{
#if __win32
  // TODO: close the duplicated stdout file descriptor here!
#endif
}

int do_read(void *buf, size_t buf_sz)
{
#if __win32
  _flushall();

  return _read(p_fds[0], buf, buf_sz);
#else
  return -1;
#endif
}

}
