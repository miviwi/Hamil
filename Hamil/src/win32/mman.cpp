#include <win32/mman.h>

#include <Windows.h>

namespace win32 {

MemoryStatus::MemoryStatus()
{
  MEMORYSTATUSEX s;
  s.dwLength = sizeof(s);

  GlobalMemoryStatusEx(&s);

  m_load = s.dwMemoryLoad;

  m_phys_size  = s.ullTotalPhys;
  m_phys_avail = s.ullAvailPhys;

  m_virt_size  = s.ullTotalVirtual;
  m_virt_avail = s.ullAvailVirtual;
}

}