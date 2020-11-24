#include <sysv/stdstream.h>

#include <config>

#if __sysv
#  include <unistd.h>
#endif

namespace sysv::stdstream_detail {

static int p_fds[2];

void init()
{
  pipe(p_fds);
  dup2(p_fds[1], 1 /* stdout */);
}

void finalize()
{
  close(p_fds[0]);
}

int do_read(void *buf, size_t buf_sz)
{
  return read(p_fds[0], buf, buf_sz);
}

 
}
