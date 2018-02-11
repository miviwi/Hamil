#include <util/format.h>

#include <cstdio>
#include <cstdarg>

namespace util {

static char p_format_string_buf[4096];

std::string fmt(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vsprintf_s(p_format_string_buf, sizeof(p_format_string_buf), fmt, va);
  va_end(va);

  return std::string(p_format_string_buf);
}

}