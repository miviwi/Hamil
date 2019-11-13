#include <sysv/stdstream.h>

#include <config>

#if __sysv
#  include <unistd.h>
#endif

namespace sysv::stdstream_detail {

static int p_fds[2];

void init()
{
#if __sysv
  pipe(p_fds);
  dup2(p_fds[1], 1 /* stdout */);
#endif
}

void finalize()
{
#if __sysv
  close(p_fds[0]);
#endif
}

int do_read(void *buf, size_t buf_sz)
{
#if __sysv
  return read(p_fds[0], buf, buf_sz);
#else
  return -1;
#endif
}

 
}
