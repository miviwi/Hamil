#include <win32/thread.h>

#include <Windows.h>

#include <utility>

namespace win32 {

DWORD Thread::ThreadProcTrampoline(void *param)
{
  auto self = (Thread *)param;

  return self->m_fn();
}

Thread::Thread(Fn fn, bool suspended) :
  m_id(0), m_fn(std::move(fn))
{
  DWORD dwCreationFlags = 0;
  dwCreationFlags |= suspended ? CREATE_SUSPENDED : 0;

  m = CreateThread(nullptr, 0, ThreadProcTrampoline, this, dwCreationFlags, &m_id);
  if(!m) throw CreateError(GetLastError());
}

Thread::Id Thread::id() const
{
  return m_id;
}

ulong Thread::exitCode() const
{
  DWORD exit_code = 0;
  if(!GetExitCodeThread(m, &exit_code)) throw Error(GetLastError());

  return exit_code;
}

}