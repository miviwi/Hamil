#include <sysv/sysv.h>
#include <sysv/cpuinfo.h>
#include <sysv/time.h>
#include <sysv/thread.h>
#include <util/ringbuffer.h>

#include <config>

#if __sysv
#  include <sys/stat.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <signal.h>
#endif

#include <exception>
#include <stdexcept>
#include <memory>
#include <vector>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace sysv {

static std::unique_ptr<cpuinfo_detail::ProcCPUInfo> p_cpuinfo;

void init_cpuinfo();
void init_signal();

void init()
{
  init_cpuinfo();
  init_signal();

  Timers::init();
}

void finalize()
{
  p_cpuinfo.release();
}

void init_cpuinfo()
{
#if __sysv
  // Passed to read() as the 'nbyte' (desired amount
  //   of bytes to read) argument
  constexpr size_t CpuinfoReadBlockSize = 1024;

  std::vector<char> cpuinfo_buf;
  cpuinfo_buf.reserve(CpuinfoReadBlockSize);

  int cpuinfo_fd = open("/proc/cpuinfo", O_RDONLY);

  // Returns 'false' when all of the data in /proc/cpuinfo
  //   has been successfully read into 'cpuinfo_buf'
  auto read_chunk = [&]() -> bool {
    cpuinfo_buf.resize(cpuinfo_buf.size() + CpuinfoReadBlockSize);
    auto buf_ptr = (cpuinfo_buf.data() + cpuinfo_buf.size()) - CpuinfoReadBlockSize;

    ssize_t bytes_read = read(cpuinfo_fd, buf_ptr, CpuinfoReadBlockSize);
    if(bytes_read < 0) throw std::runtime_error("read() error!");   // Should be unreachable under
                                                                    //   regular circumstances

    // No more data to read
    if(bytes_read < CpuinfoReadBlockSize) {
      size_t proper_size = (cpuinfo_buf.size() - CpuinfoReadBlockSize) + (size_t)bytes_read;
      cpuinfo_buf.resize(proper_size + 1 /* space for '\0' */);

      return false;
    }

    return true;
  };

  // Read all of /proc/cpuinfo into 'cpuinfo_buf'
  while(read_chunk()) ;

  // Cleanup
  close(cpuinfo_fd);

  // Parse the /proc/cpuinfo data and store it for later
  p_cpuinfo = cpuinfo_detail::ProcCPUInfo::parse_proc_cpuinfo(cpuinfo_buf.data());
#endif
}

namespace cpuinfo_detail {

ProcCPUInfo *cpuinfo()
{
  assert(p_cpuinfo && "sysv::cpuinfo_detail::cpuinfo() called before sysv::init()!");

  return p_cpuinfo.get();
}

}

namespace thread_detail {

#if __sysv
const int ThreadSuspendResumeSignal = SIGUSR1;

static void thread_suspend_resume_sigaction(int sig, siginfo_t *info, void *ucontext)
{
  auto action = (ThreadSuspendResumeAction *)info->si_value.sival_ptr;

  sysv::Thread::thread_suspend_resume(action);
}
#else
const int ThreadSuspendResumeSignal = -1;
#endif

}

void init_signal()
{
#if __sysv
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));

  sa.sa_sigaction = thread_detail::thread_suspend_resume_sigaction;
  
  // Mask all signals...
  auto fill_error = sigfillset(&sa.sa_mask);
  assert(!fill_error);

  // ...except the ThreadSuspendResumeSignal itself (i.e. SIGUSR1)
  auto del_error = sigdelset(&sa.sa_mask, thread_detail::ThreadSuspendResumeSignal);
  assert(!del_error);

  sa.sa_flags = SA_SIGINFO;

  auto result = sigaction(thread_detail::ThreadSuspendResumeSignal, &sa, nullptr);
  if(result < 0) {
    perror("sigaction()");
    exit(1);
  }
#endif
}

}
