#include <win32/thread.h>

#include <Windows.h>

#include <utility>

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

// Source: MSDN - How to: Set a Thread Name in Native Code
static void SetThreadName(DWORD dwThreadID, const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

#pragma warning(push)
#pragma warning(disable: 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}

namespace win32 {

class ThreadData {
public:
  Thread::Id id;
  Thread::Fn fn;
  Thread::OnExit on_exit;

protected:
  ThreadData() = default;

private:
  friend Thread;
};

DWORD Thread::ThreadProcTrampoline(void *param)
{
  // The lpParameter is a pointer to this Thread's m_data
  auto self = (ThreadData *)param;

  // Actually execute the Thread's code
  DWORD exit_code = self->fn();

  // Call the OnExit handler if it has been set
  if(auto& on_exit = self->on_exit) {
    on_exit();
  }

  return exit_code;
}

Thread::Thread(Fn fn, bool suspended) :
  m_data(new ThreadData())
{
  DWORD dwCreationFlags = 0;
  dwCreationFlags |= suspended ? CREATE_SUSPENDED : 0;

  m_data->id = InvalidId;  // Will be initialized by CreateThread
  m_data->fn = std::move(fn);

  m = CreateThread(nullptr, 0, ThreadProcTrampoline, m_data, dwCreationFlags, &m_data->id);
  if(!m) throw CreateError(GetLastError());
}

Thread::~Thread()
{
  // Only CHECK the ref-count so it's not decremented twice
  //   - See the comment above Waitable
  if(refs() > 1) return;

  // A count == 1 means somebody down the line will decrement
  //   it (i.e. set it to 0), thus the object is already dead
  //   and we need to clean it up
  delete m_data;
}

Thread::Id Thread::id() const
{
  return m_data->id;
}

Thread& Thread::dbg_SetName(const char *name)
{
#if !defined(NDEBUG)
  SetThreadName(m_data->id, name);
#endif

  return *this;
}

Thread::Id Thread::current_thread_id()
{
  return GetCurrentThreadId();
}

Thread& Thread::resume()
{
  ResumeThread(m);

  return *this;
}

Thread& Thread::suspend()
{
  SuspendThread(m);

  return *this;
}

Thread& Thread::affinity(uintptr_t mask)
{
  SetThreadAffinityMask(m, mask);

  return *this;
}

ulong Thread::exitCode() const
{
  DWORD exit_code = 0;
  if(!GetExitCodeThread(m, &exit_code)) throw Error(GetLastError());

  return exit_code;
}

Thread& Thread::onExit(OnExit on_exit)
{
  m_data->on_exit = on_exit;

  return *this;
}

}