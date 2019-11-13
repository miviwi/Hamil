#include <os/os.h>
#include <os/cpuid.h>
#include <os/cpuinfo.h>
#include <os/stdstream.h>

#include <win32/win32.h>
#include <sysv/sysv.h>

#include <config>

#include <memory>

#include <cassert>

namespace os {

static std::unique_ptr<CpuInfo> p_cpuinfo;

void init()
{
#if __win32
  win32::init();
#elif __sysv
  sysv::init();
#else
#  error "unknown platform"
#endif

  p_cpuinfo.reset(CpuInfo::create());
}

void finalize()
{
#if __win32
  win32::finalize();
#elif __sysv
  sysv::finalize();
#else
#  error "unknown platform"
#endif

  p_cpuinfo.release();
}

CpuInfo& cpuinfo()
{
  assert(p_cpuinfo && "os::cpuinfo() called before os::init()!");

  return *p_cpuinfo;
}

}
