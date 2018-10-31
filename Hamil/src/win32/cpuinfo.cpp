#include <win32/cpuinfo.h>

#include <Windows.h>

#include <cstdlib>

namespace win32 {

uint CpuInfo::numPhysicalProcessors() const
{
  return m_num_physical_processors;
}

uint CpuInfo::numLogicalProcessors() const
{
  return m_num_logical_processors;
}

template <typename T>
static uint p_count_bits(T x)
{
  uint count = 0;
  for(size_t i = 0; i < sizeof(T)*CHAR_BIT; i++) {
    count += x&1 ? 1 : 0;
    x >>= 1;
  }

  return count;
}

CpuInfo *CpuInfo::create()
{
  auto self = new CpuInfo();

  DWORD info_len = 0;
  GetLogicalProcessorInformation(nullptr, &info_len);

  auto info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(info_len);
  GetLogicalProcessorInformation(info, &info_len);

  size_t offset = 0;
  while(offset+sizeof(*info) < info_len) {
    auto ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)((byte *)info + offset);
    switch(ptr->Relationship) {
    case RelationProcessorCore:
      self->m_num_physical_processors++;

      self->m_num_logical_processors += p_count_bits(ptr->ProcessorMask);
      break;
    }

    offset += sizeof(*info);
  }
  free(info);

  return self;
}

}