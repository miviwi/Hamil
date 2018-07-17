#pragma once

#include <common.h>

namespace win32 {

struct MemoryStatus {
public:
  MemoryStatus();

  unsigned loadPercentage() const { return m_load; }

  size_t physicalSize()      const { return m_phys_size; }
  size_t physicalAvailable() const { return m_phys_avail; }

  size_t virtualSize()      const { return m_virt_size; }
  size_t virtualAvailable() const { return m_virt_avail; }

private:
  unsigned m_load;

  size_t m_phys_size;
  size_t m_phys_avail;

  size_t m_virt_size;
  size_t m_virt_avail;
};

}