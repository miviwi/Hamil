#include <os/stdstream.h>

#include <win32/stdstream.h>
#include <sysv/stdstream.h>

#include <config>

#include <cassert>
#include <cstring>
#include <cstdio>

namespace os {

static constexpr size_t p_buf_sz = 1024*1024; // 1MB
static char *p_buf = nullptr;

void StdStream::init()
{
  p_buf = new char[p_buf_sz];
  memset(p_buf, 0, p_buf_sz);    // new[] zero-initializes the returned array,
                                 //   but just to be sure...
}

void StdStream::finalize()
{
#if __win32
  win32::stdstream_detail::finalize();
#elif __sysv
  sysv::stdstream_detail::finalize();
#else
#  error "unknown platform"
#endif

  delete[] p_buf;
}

std::string StdStream::gets()
{
  printf("\r\n"); // Make sure there's something in the buffer so we don't block on _read()
  fflush(stdout);

#if __win32
  int length = win32::stdstream_detail::do_read(
      p_buf, p_buf_sz-1 /* the buffer holds a NULL trminated string */);
#elif __sysv
  int length = sysv::stdstream_detail::do_read(
      p_buf, p_buf_sz-1 /* the buffer holds a NULL trminated string */);
#else
#  error "unknown platform"
#endif

  return std::string(p_buf, length-2); // Remove the superfluous CRLF added above
}

}
