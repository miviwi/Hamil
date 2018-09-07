#include <win32/event.h>

#include <Windows.h>

namespace win32 {

Event::Event(bool state, unsigned flags, const char *name)
{
  bool bManualReset = flags & ManualReset;
  m = CreateEventA(nullptr, bManualReset, state, name);

  if(!m) throw CreateError(GetLastError());
}

void Event::set()
{
  if(!SetEvent(m)) throw SetResetError(GetLastError());
}

void Event::reset()
{
  if(!ResetEvent(m)) throw SetResetError(GetLastError());
}

}