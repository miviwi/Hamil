#include <win32/event.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

namespace win32 {

Event::Event(bool state, unsigned flags, const char *name)
{
#if defined(_MSVC_VER)
  bool bManualReset = flags & ManualReset;
  m = CreateEventA(nullptr, bManualReset, state, name);

  if(!m) throw CreateError(GetLastError());
#endif
}

void Event::set()
{
#if defined(_MSVC_VER)
  if(!SetEvent(m)) throw SetResetError(GetLastError());
#endif
}

void Event::reset()
{
#if defined(_MSVC_VER)
  if(!ResetEvent(m)) throw SetResetError(GetLastError());
#endif
}

}
