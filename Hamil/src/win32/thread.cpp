#include <win32/thread.h>

#include <config>

#if __win32
#  include <Windows.h>
#else
#  define DWORD ulong
#endif

#include <new>
#include <utility>

#include <cassert>

#if __win32
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
#else

static void SetThreadName(DWORD dwThreadID, const char *name)
{
}

#endif

namespace win32 {

class ThreadData {
public:
#if __win32
  HANDLE handle;
#endif

  Thread::Id id;

  Thread::Fn fn;
};

DWORD Thread::ThreadProcTrampoline(void *param)
{
  // The lpParameter is a pointer to this Thread's m_data
  auto self = (ThreadData *)param;

  // Actually execute the Thread's code
  DWORD exit_code = self->fn();

  // Call the OnExit handler if it has been set
#if 0
  if(auto& on_exit = self->on_exit) {
    on_exit();
  }
#endif

  return exit_code;
}

Thread::Thread() :
  os::Thread(sizeof(ThreadData))
{
  new(storage<ThreadData>()) ThreadData();
}

Thread::~Thread()
{
  // Only CHECK the ref-count so it's not decremented twice
  if(refs() > 1) return;

  assert(Thread::exitCode() != StillActive &&
      "at least ONE reference to a Thread object must be kept around while it's running!");

  storage<ThreadData>()->~ThreadData();
}

Thread::Id Thread::id() const
{
  return data().id;
}

os::Thread& Thread::dbg_SetName(const char *name)
{
#if !__win32
  SetThreadName(id(), name);
#endif

  return *this;
}

#if 0
Thread::Id Thread::current_thread_id()
{
#if __win32
  return GetCurrentThreadId();
#else
  return InvalidId;
#endif
}
#endif

os::Thread& Thread::resume()
{
#if __win32
  ResumeThread(m);
#endif

  return *this;
}

os::Thread& Thread::suspend()
{
#if __win32
  SuspendThread(m);
#endif

  return *this;
}

os::Thread& Thread::affinity(uintptr_t mask)
{
#if __win32
  SetThreadAffinityMask(m, mask);
#endif

  return *this;
}

u32 Thread::exitCode()
{
  assert(id() != InvalidId && "a Thread must've had create() called on it before calling exitCode()!");

#if __win32
  DWORD exit_code = 0;
  if(!GetExitCodeThread(data().handle, &exit_code)) throw Error(GetLastError());

  return exit_code == STILL_ACTIVE ? StillActive : exit_code;
#else
  return StillActive;
#endif
}

os::WaitResult Thread::wait(ulong timeout)
{
  assert(0 && "win32::Thread::wait() unimplemented!");

  return os::WaitFailed;
}

void Thread::doCreate(Fn fn, bool suspended)
{
#if __win32
  DWORD dwCreationFlags = 0;
  dwCreationFlags |= suspended ? CREATE_SUSPENDED : 0;

  DWORD id;
  auto handle = CreateThread(
      nullptr, 0,
      ThreadProcTrampoline, storage(),
      dwCreationFlags, &id
  );
  if(!handle) throw CreateError(GetLastError());

  data().handle = handle;

  data().id = (Id)id;
  data().fn = fn;

#endif
}

ThreadData& Thread::data()
{
  return *storage<ThreadData>();
}

const ThreadData& Thread::data() const
{
  return *storage<ThreadData>();
}

}
