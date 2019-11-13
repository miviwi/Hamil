#include <os/cpuinfo.h>

#include <win32/cpuinfo.h>
#include <sysv/cpuinfo.h>

#include <config>

namespace os {

uint CpuInfo::numPhysicalProcessors() const
{
  return m_num_physical_processors;
}

uint CpuInfo::numLogicalProcessors() const
{
  return m_num_logical_processors;
}

bool CpuInfo::hyperthreading() const
{
  return m_num_logical_processors > m_num_physical_processors;
}

CpuInfo *CpuInfo::create()
{
#if __win32
  return win32::create_cpuinfo();
#elif __sysv
  return sysv::create_cpuinfo();
#else
#  error "unknown platform"
#endif
}

}
